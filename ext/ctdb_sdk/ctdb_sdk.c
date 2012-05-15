#include "ruby.h"
#include "ctdbsdk.h" /* c-tree headers */

#define CTH(obj) \
    (Check_Type(obj, T_DATA), &(((struct ctree*)DATA_PTR(obj))->handle))
#define CTRecord(obj) \
    (Check_Type(obj, T_DATA), (struct ctree_record*)DATA_PTR(obj))
#define CTRecordH(obj) \
    (Check_Type(obj, T_DATA), &(((struct ctree_record*)DATA_PTR(obj))->handle))
// #define CTType(obj) \
//     (Check_Type(obj, T_DATA), &(((struct ctree_type*)DATA_PTR(obj))->val))
#define CTDate(obj) \
    (Check_Type(obj, T_DATA), (struct ctree_date*)DATA_PTR(obj))
    // (Check_Type(obj, T_DATA), (((struct ctree_date*)DATA_PTR(obj))->val)))

struct ctree {
    CTHANDLE handle;
};

struct ctree_record {
    CTHANDLE handle;
    pCTHANDLE table_ptr;
};

/* TODO: Refactor CT types to use one structure */
// struct ctree_type {
//     void *val;
// };

struct ctree_date {
    CTDATE val;
};

VALUE mCT;
VALUE cCTError;    // CT::Error
VALUE cCTSession;  // CT::Session
VALUE cCTTable;    // CT::Table
VALUE cCTIndex;    // CT::Index
VALUE cCTSegment;  // CT::Segment
VALUE cCTRecord;   // CT::Record
VALUE cCTField;    // CT::Field
VALUE cCTDate;     // CT::Date

VALUE rb_ctdb_field_new(VALUE klass, CTHANDLE field);
VALUE rb_ctdb_index_new(VALUE klass, CTHANDLE index);

// ============================
// = CT::Session && CT::Table =
// ============================

/*
 * Retrieve the default date type.
 * 
 * @return [Fixnum] The date type.
 * @raise [CT::Error] ctdbGetDefDateType failed.
 */
static VALUE
rb_ctdb_get_defualt_date_type(VALUE self)
{
    pCTHANDLE cth = CTH(self);
    CTDATE_TYPE t;

    if((t = ctdbGetDefDateType(*cth)) == 0)
        rb_raise(cCTError, "[%d] ctdbGetDefDateType failed.", ctdbGetError(*cth));

    return INT2FIX(t);
}

/*
 * Set the default date type.
 *
 * @param [Fixnum] type
 * @raise [CT::Error] ctdbSetDefDateType failed.
 */
static VALUE
rb_ctdb_set_defualt_date_type(VALUE self, VALUE type)
{
    Check_Type(type, T_FIXNUM);

    pCTHANDLE cth = CTH(self);

    if(ctdbSetDefDateType(*cth, FIX2INT(type)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetDefDateType failed", ctdbGetError(*cth));

    return self;
}

// ===============
// = CT::Session =
// ===============
static void 
free_rb_ctdb_session(struct ctree* ct)
{
    ctdbFreeSession(ct->handle);
    free(ct);
}

VALUE
rb_ctdb_session_new(VALUE klass, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    struct ctree* ct;
    VALUE obj = Data_Make_Struct(klass, struct ctree, 0, free_rb_ctdb_session, ct);

    // Allocate a new session for logon only. No session or database dictionary
    // files will be used. No database functions can be used with this session mode.
    if((ct->handle = ctdbAllocSession(FIX2INT(mode))) == NULL)
        rb_raise(cCTError, "ctdbAllocSession failed.");

    rb_obj_call_init(obj, 0, NULL); // CT::Session.initialize

    return obj;
}

/*
 * Retrieve the active state of a table.  A table is active if it is open.
 */
static VALUE 
rb_ctdb_session_is_active(VALUE self)
{
    return ctdbIsActiveSession(*CTH(self)) ? Qtrue : Qfalse;
}

/*
 * Perform a session-wide lock.
 *
 * @param [Fixnum] mode A c-treeDB lock mode.
 */
static VALUE
rb_ctdb_session_lock(VALUE self, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);
    return ctdbLock(*CTH(self), FIX2INT(mode)) == CTDBRET_OK ? Qtrue : Qfalse;
}

/*
 * @see #lock
 * @raise [CT::Error] ctdbLock failed.
 */
static VALUE
rb_ctdb_session_lock_bang(VALUE self, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    pCTHANDLE cth = CTH(self);

    if(ctdbLock(*cth, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbLock failed.", ctdbGetError(*cth));

    return Qtrue;
}

/* 
 * Check to see if a lock is active
 */
static VALUE
rb_ctdb_session_is_locked(VALUE self)
{
    return ctdbIsLockActive(*CTH(self)) == YES ? Qtrue : Qfalse;
}

/*
 * Logon to c-tree Server or c-treeACE instance session.
 *
 * @param [String] host c-tree Server name or c-treeACE instance name.
 * @param [String] username 
 * @param [String] password
 * @raise [CT::Error] ctdbLogon failed.
 */
static VALUE 
rb_ctdb_session_logon(int argc, VALUE *argv, VALUE self)
{
    VALUE host, user, pass;
    pCTHANDLE cth = CTH(self);

    rb_scan_args(argc, argv, "30", &host, &user, &pass);
    char *h = RSTRING_PTR(host);
    char *u = RSTRING_PTR(user);
    char *p = RSTRING_PTR(pass);

    if(ctdbLogon(*cth, h, u, p) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbLogon failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Logout from a c-tree Server session or from a c-treeACE instance.
 */
static VALUE 
rb_ctdb_session_logout(VALUE self)
{
    ctdbLogout(*CTH(self));
    return self;
}

/*
 * Return the user password associated with the session.
 *
 * @return [String]
 * @raise [CT::Error] ctdbGetUserPassword failed.
 */
static VALUE
rb_ctdb_session_get_password(VALUE self)
{
    pTEXT pword;
    pCTHANDLE cth = CTH(self);

    if((pword = ctdbGetUserPassword(*cth)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetUserPassword failed.", ctdbGetError(*cth));

    return rb_str_new2(pword);
}

/*
 * Returns the client-side path prefix.
 * 
 * @return [String, nil] The path name or nil if no path is set.
 */
static VALUE
rb_ctdb_session_get_path_prefix(VALUE self)
{
    pTEXT prefix = ctdbGetPathPrefix(*CTH(self));
    return prefix ? rb_str_new2(prefix) : Qnil;
}

/*
 * Set the client-side path prefix.
 *
 * @param [String] prefix Path.
 * @raise [CT::Error] ctdbSetPathPrefix failed.
 */
static VALUE
rb_ctdb_session_set_path_prefix(VALUE self, VALUE prefix)
{
    pTEXT pp;

    switch(rb_type(prefix)){
        case T_STRING :
            pp = ((RSTRING_LEN(prefix) == 0) ? NULL : RSTRING_PTR(prefix));
            break;
        case T_NIL :
            pp = NULL;
            break;
        default :
            rb_raise(rb_eArgError, "Unexpected value type `%s'", 
                rb_obj_classname(prefix));
            break;
    }

    pCTHANDLE cth = CTH(self);
    if(ctdbSetPathPrefix(*cth, pp) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetPathPrefix failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Releases all session-wide locks
 *
 * @return [Boolean]
 */
static VALUE
rb_ctdb_session_unlock(VALUE self)
{
    return ctdbUnlock(*CTH(self)) == CTDBRET_OK ? Qtrue : Qfalse;
}

/* 
 * @see #unlock
 * @raise [CT::Error] ctdbUnlock failed.
 */
static VALUE
rb_ctdb_session_unlock_bang(VALUE self)
{
    pCTHANDLE cth = CTH(self);

    if(ctdbUnlock(*cth) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbUnlock failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Return the user name associated with the session.
 *
 * @return [String]
 * @raise [CT::Error] ctdbGetUserLogonName failed.
 */
static VALUE
rb_ctdb_session_get_username(VALUE self)
{
    pTEXT name;
    pCTHANDLE cth = CTH(self);

    if((name = ctdbGetUserLogonName(*cth)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetUserLogonName failed.", ctdbGetError(*cth));

    return rb_str_new2(name);
}

// =============
// = CT::Table =
// =============
static void
free_rb_ctdb_table(struct ctree* ct)
{
    ctdbFreeTable(ct->handle);
    free(ct);
}

/*
 * @param [CT::Session]
 */
VALUE
rb_ctdb_table_new(VALUE klass, VALUE session)
{
    Check_Type(session, T_DATA);

    struct ctree* ct;
    pCTHANDLE cth = CTH(session);
    VALUE obj = Data_Make_Struct(klass, struct ctree, 0, free_rb_ctdb_table, ct);

    if((ct->handle = ctdbAllocTable(*cth)) == NULL)
        rb_raise(cCTError, "[%d] ctdbAllocTable failed", ctdbGetError(*cth));

    VALUE argv[1] = { session };
    rb_obj_call_init(obj, 1, argv); // CT::Table.initialize(session)

    return obj;
}

static VALUE
rb_ctdb_table_init(VALUE self, VALUE session)
{
    return self;
}

/*
 * Add a new field to the table.
 *
 * @param [String] name Field name
 * @param [Fixnum] type Field type
 * @param [Integer] length Field length
 * @param [Hash] opts Optional field flags.
 * @options opts [Boolean] :allow_null Set the field null flag.
 * @options opts [Object] :default Set the fields default value.
 * @options opts [Fixnum] :scale The number of digits to the right of the 
 * decimal point.
 * @options opts [Fixnum] :precision The maximun number of digits.
 * @raise [CT::Error]
 */
static VALUE
rb_ctdb_table_add_field(VALUE self, VALUE name, VALUE type, VALUE length, VALUE opts)
{
    Check_Type(name, T_STRING);
    Check_Type(type, T_FIXNUM);
    Check_Type(length, T_FIXNUM);
    if(!opts == Qnil) Check_Type(opts, T_HASH);

    pCTHANDLE cth = CTH(self);
    CTHANDLE field;

    field = ctdbAddField(*cth, RSTRING_PTR(name), FIX2INT(type), FIX2INT(length));
    if(!field) rb_raise(cCTError, "[%d] ctdbAddField failed.", ctdbGetError(*cth));

    // struct* ct;
    VALUE ct_field = rb_ctdb_field_new(cCTField, field);
    // ct_field = Data_Make_Struct(cCTField, struct ctree, 0, free_rb_ctdb_field, ct);
    // ct_field->handle = field;

    return ct_field;
}

/*
 * Add a new index the the table.
 *
 * @param [String] name Index name
 * @param [Fixnum] type Key type
 * @param [Hash] opts
 * @options [Boolean] :allow_dups Indication if the index allows duplicate keys.
 * @options [Boolean] :allow_null Indidication if the index allows null keys.
 * @raise [CT::Error] ctdbAddIndex failed.
 */
static VALUE
rb_ctdb_table_add_index(VALUE self, VALUE name, VALUE type)/*, VALUE opts)*/
{
    Check_Type(name, T_STRING);
    // Check_Type(type, T_DATA);
    // Check_Type(opts, T_HASH);

    // if(rb_type(allow_dups) != T_TRUE && rb_type(allow_dups) != T_FALSE)
    //     rb_raise(rb_eArgError, "Unexpected value type `%s' for allow_dups", 
    //              rb_obj_classname(allow_dups));

    // if(rb_type(allow_null) != T_TRUE && rb_type(allow_null) != T_FALSE)
    //     rb_raise(rb_eArgError, "Unexpected value type `%s' for allow_null", 
    //             rb_obj_classname(allow_null));

    CTHANDLE index;
    pCTHANDLE cth = CTH(self);
    // CTBOOL dflag  = (rb_type(allow_dups) == T_TRUE ? YES : NO);
    // CTBOOL nflag  = (rb_type(allow_null) == T_TRUE ? YES : NO);
    CTBOOL dflag = NO;
    CTBOOL nflag = YES;

    index = ctdbAddIndex(*cth, RSTRING_PTR(name), FIX2INT(type), dflag, nflag);
    if(!index) rb_raise(cCTError, "[%d] ctdbAddIndex failed.", ctdbGetError(*cth));

    VALUE ct_index = rb_ctdb_index_new(cCTIndex, index);

    return ct_index;
}

/*
 * Rebuild existing table based on field, index and segment changes.
 *
 * @param [Fixnum] mode The alter table action
 * @raise [CT::Error] ctdbAlterTable failed.
 */
static VALUE
rb_ctdb_table_alter(VALUE self, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    pCTHANDLE cth = CTH(self);

    if(ctdbAlterTable(*cth, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbAlterTable failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Close the table.
 *
 * @raise [CT::Error] ctdbCloseTable failed.
 */
static VALUE
rb_ctdb_table_close(VALUE self)
{
    pCTHANDLE cth = CTH(self);

    if(ctdbCloseTable(*cth) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbCloseTable failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Create a new table
 *
 * @param [Strng] name The table name
 * @param [Fixnum] mode Table create mode.
 * @raise [CT::Error] ctdbCreateTable failed.
 */
static VALUE
rb_ctdb_table_create(VALUE self, VALUE name, VALUE mode)
{
    Check_Type(name, T_STRING);
    Check_Type(mode, T_FIXNUM);

    pCTHANDLE cth = CTH(self);

    // if(ctdbCreateTable(*cth, RSTRING_PTR(name), FIX2INT(mode)) != CTDBRET_OK)
    if(ctdbCreateTable(*cth, RSTRING_PTR(name), CTCREATE_NORMAL) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbCreateTable failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retrieve the table delimiter character.
 *
 * @return [String] The field delimiter character.
 * @raise [CT::Error] ctdbGetPadChar failed.
 */
static VALUE rb_ctdb_table_get_delim_char(VALUE self)
{
    pCTHANDLE cth = CTH(self);
    pTEXT dchar;

    if(ctdbGetPadChar(*cth, NULL, dchar) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetPadChar failed.", ctdbGetError(*cth));

    return rb_str_new2(dchar);
}

/*
 * Retrieve a table field based on the field number.
 *
 * @param [Fixnum] index The field number.
 * @return [CT::Field]
 * @raise [CT::Error] ctdbGetField failed.
 */
static VALUE
rb_ctdb_table_get_field(VALUE self, VALUE index)
{
    Check_Type(index, T_FIXNUM);

    pCTHANDLE cth = CTH(self);
    CTHANDLE field;

    if((field = ctdbGetField(*cth, FIX2INT(index))) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetField failed.", ctdbGetError(*cth));

    return rb_ctdb_field_new(cCTField, field);
}

/*
 * Retrieve all fields in a table.
 *
 * @yield [field] Each table as a CT::Field object.
 * @return [Array] Collection of CT::Field objects.
 * @raise [CT::Error] ctdbGetTableFieldCount or ctdbGetField failed.
 */
static VALUE
rb_ctdb_table_get_fields(VALUE self)
{
    int i, n;
    pCTHANDLE cth = CTH(self);
    CTHANDLE field;
    pTEXT fname;
    VALUE fields;

    if((n = ctdbGetTableFieldCount(*cth)) == -1)
        rb_raise(cCTError, "[%d] ctdbGetTableFieldCount failed.", 
            ctdbGetError(*cth));

    fields = rb_ary_new2(n);

    for(i = 0; i < n; i++){
        // Get the field handle.
        if((field = ctdbGetField(*cth, i)) == NULL)
            rb_raise(cCTError, "[%d] ctdbGetField failed.", ctdbGetError(*cth));

        VALUE ct_field = rb_ctdb_field_new(cCTField, field);
        if(rb_block_given_p()) rb_yield(ct_field);
        rb_ary_store(fields, i, rb_str_new2(ct_field));
    }

    return fields;
}

/*
 * Retrieve a table field based on the field name.
 *
 * @param [String] name The field name.
 * @return [CT::Field]
 * @raise [CT::Error] ctdbGetFieldByName failed.
 */
static VALUE
rb_ctdb_table_get_field_by_name(VALUE self, VALUE name)
{
    Check_Type(name, T_STRING);

    pCTHANDLE cth = CTH(self);
    CTHANDLE field;

    if((field = ctdbGetFieldByName(*cth, RSTRING_PTR(name))) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldByName failed.", ctdbGetError(*cth));

    return rb_ctdb_field_new(cCTField, field);
}

/*
 * Retrieve the number of fields associated with the table. 
 *
 * @return [Fixnum]
 * @raise [CT::Error] ctdbGetTableFieldCount failed.
 */
static VALUE
rb_ctdb_table_get_field_count(VALUE self)
{
    int i;
    pCTHANDLE cth = CTH(self);

    if((i = ctdbGetTableFieldCount(*cth)) == -1)
        rb_raise(cCTError, "[%d] ctdbGetTableFieldCount failed.", 
            ctdbGetError(*cth));

     return INT2FIX(i);
}

/*
 * Retrieve a collection of all the field names associated with the table.
 *
 * @return [Array]
 * @raise [CT::Error] ctdbGetTableFieldCount, ctdbGetField, or ctdbGetFieldName 
 *        failed.
 */
static VALUE
rb_ctdb_table_get_field_names(VALUE self)
{
    int i, n;
    pCTHANDLE table = CTH(self);
    CTHANDLE field;
    pTEXT fname;
    VALUE fields;

    if((n = ctdbGetTableFieldCount(*table)) == -1)
        rb_raise(cCTError, "[%d] ctdbGetTableFieldCount failed.", 
            ctdbGetError(*table));

    fields = rb_ary_new2(n);

    for(i = 0; i < n; i++){
        // Get the field handle.
        if((field = ctdbGetField(*table, i)) == NULL)
            rb_raise(cCTError, "[%d] ctdbGetField failed.", 
                ctdbGetError(*table));
        // Use the field handle to retrieve the field name.
        if((fname = ctdbGetFieldName(field)) == NULL)
            rb_raise(cCTError, "[%d] ctdbGetFieldName failed.", 
                ctdbGetError(*table));
        // Store the field name
        rb_ary_store(fields, i, rb_str_new2(fname));
    }
    return fields;
}

/*
 * Retrieve the table name.
 *
 * @return [String, nil]
 */
static VALUE
rb_ctdb_table_get_name(VALUE self)
{
    VALUE s = rb_str_new2(ctdbGetTableName(*CTH(self)));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

/*
 * Open the table.
 *
 * @param [String] name Name of the table to open
 * @raise [CT::Error] ctdbOpenTable failed.
 */
static VALUE
rb_ctdb_table_open(VALUE self, VALUE name, VALUE mode)
{
    Check_Type(name, T_STRING);

    pCTHANDLE cth = CTH(self);

    // if(ctdbOpenTable(*cth, RSTRING_PTR(name), FIX2INT(mode)) !=  CTDBRET_OK)
    if(ctdbOpenTable(*cth, RSTRING_PTR(name), CTOPEN_NORMAL) !=  CTDBRET_OK)
        rb_raise(cCTError, "[%d][%d] ctdbOpenTable failed.", 
                ctdbGetError(*cth), sysiocod);

    return self;
}

/*
 * Retrieve the table pad character
 *
 * @return [String] The character.
 * @raise [CT::Error] ctdbGetPadChar failed
 */
static VALUE
rb_ctdb_table_get_pad_char(VALUE self)
{
    pCTHANDLE cth = CTH(self);
    pTEXT pchar;

    if(ctdbGetPadChar(*cth, pchar, NULL) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetPadChar failed.", ctdbGetError(*cth));

    return rb_str_new2(pchar);
}

/*
 * Retrieve the table path.
 *
 * @return [String, nil] The path or nil if one is not set.
 */
static VALUE
rb_ctdb_table_get_path(VALUE self)
{
    VALUE s = rb_str_new2(ctdbGetTablePath(*CTH(self)));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

/*
 * Set a new table path
 *
 * @param [String] path File path to the table.
 * @raise [CT::Error] ctdbSetTablePath failed.
 */
static VALUE
rb_ctdb_table_set_path(VALUE self, VALUE path)
{
    Check_Type(path, T_STRING);

    pCTHANDLE cth = CTH(self);

    if(ctdbSetTablePath(*cth, RSTRING_PTR(path)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetTablePath failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retrieves the table status. The table status indicates which rebuild action 
 * will be taken by an alter table operation.
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_table_get_status(VALUE self)
{
    return INT2FIX(ctdbGetTableStatus(*CTH(self)));
}

// =============
// = CT::Field =
// =============
static void
free_rb_ctdb_field(struct ctree* ct)
{
    free(ct);
}

VALUE
rb_ctdb_field_new(VALUE klass, CTHANDLE field)
{
    struct ctree* ct;
    VALUE obj = Data_Make_Struct(klass, struct ctree, 0, free_rb_ctdb_field, ct);
    ct->handle = field;

    rb_obj_call_init(obj, 0, NULL); // CT::Field.initialize

    return obj;
}

/*
 *
 */
static VALUE
rb_ctdb_field_get_null_flag(VALUE self)
{
    return ctdbGetFieldNullFlag(*CTH(self)) == YES ? Qtrue : Qfalse;
}

static VALUE
rb_ctdb_field_get_default(VALUE self)
{
    return self;
}

static VALUE
rb_ctdb_field_set_default(VALUE self, VALUE value)
{
    return self;
}

/*
 * Retrieve the field length.
 *
 * @return [Fixnum] The field length.
 * @raise [CT::Error] ctdbGetFieldLength failed.
 */
static VALUE
rb_ctdb_field_get_length(VALUE self)
{
    pCTHANDLE cth = CTH(self);
    VRLEN length;
    if((length = ctdbGetFieldLength(*cth)) == -1)
        rb_raise(cCTError, "[%d] ctdbGetFieldLength failed.", ctdbGetError(*cth));

    return INT2FIX(length);
}

/*
 * Set the field length.
 *
 * @param [Fixnum] length The field length.
 * @raise [CT::Error] ctdbSetFieldLength failed.
 */
static VALUE
rb_ctdb_field_set_length(VALUE self, VALUE length)
{
    Check_Type(length, T_FIXNUM);

    pCTHANDLE cth = CTH(self);

    if(ctdbSetFieldLength(*cth, FIX2INT(length)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldLength failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retreive the field name.
 *
 * @return [String]
 * @raise [CT::Error] ctdbGetFieldName failed.
 */
static VALUE
rb_ctdb_field_get_name(VALUE self)
{
    pCTHANDLE cth = CTH(self);
    pTEXT name;

    if((name = ctdbGetFieldName(*cth)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetFieldName failed.", ctdbGetError(*cth));

    return rb_str_new2(name);
}

/*
 * Set the new field name.
 *
 * @param [String] name The field name.
 * @raise [CT::Error] ctdbSetFieldName failed.
 */
static VALUE
rb_ctdb_field_set_name(VALUE self, VALUE name)
{
    Check_Type(name, T_STRING);

    pCTHANDLE cth = CTH(self);

    if(ctdbSetFieldName(*cth, RSTRING_PTR(name)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldName failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retreive the fields precision. The field precision represents the total number 
 * of digits in a BCD number.
 * 
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_field_get_precision(VALUE self)
{
    return INT2FIX(ctdbGetFieldPrecision(*CTH(self)));
}

/*
 * Set the field precision (maximun number of digits).
 *
 * @raise [CT::Error] ctdbSetFieldPrecision failed.
 */
static VALUE
rb_ctdb_field_set_precision(VALUE self, VALUE precision)
{
    Check_Type(precision, T_FIXNUM);

    pCTHANDLE cth = CTH(self);

    if(ctdbSetFieldPrecision(*cth, FIX2INT(precision)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldPrecision failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retrieve the field scale.  This represents the number of digits to the right
 * of the decimal point.
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_field_get_scale(VALUE self)
{
    return INT2FIX(ctdbGetFieldScale(*CTH(self)));
}

static VALUE
rb_ctdb_field_set_scale(VALUE self, VALUE scale)
{
    return self;
}

static VALUE
rb_ctdb_field_get_type(VALUE self)
{
    return self;
}

static VALUE
rb_ctdb_field_set_type(VALUE self, VALUE type)
{
    return self;
}


// =============
// = CT::Index =
// =============
static void
free_rb_ctdb_index(struct ctree* ct)
{
    free(ct);
}

VALUE
rb_ctdb_index_new(VALUE klass, CTHANDLE index)
{
    struct ctree* ct;
    VALUE obj = Data_Make_Struct(klass, struct ctree, 0, free_rb_ctdb_index, ct);
    ct->handle = index;

    rb_obj_call_init(obj, 0, NULL); // CT::Index.initialize

    return obj;
}

static VALUE
rb_ctdb_index_add_segment(VALUE self, VALUE field, VALUE mode)
{
    Check_Type(field, T_DATA);
    pCTHANDLE cth = CTH(self);

    if(!ctdbAddSegment(*cth, *CTH(field), CTSEG_SCHSEG))
        rb_raise(cCTError, "[%d] ctdbAddSegment failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retrieve the key type for this index.
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_index_get_key_type(VALUE self)
{
    return INT2FIX(ctdbGetIndexKeyType(*CTH(self)));
}

/*
 * Retrieve the index name
 *
 * @return [String, nil]
 */
static VALUE
rb_ctdb_index_get_name(VALUE self)
{
    pTEXT name = ctdbGetIndexName(*CTH(self));
    return name ? rb_str_new2(name) : Qnil;
}

/*
 * Retrieve the key length for this index.
 *
 * @return [Fixnum, nil]
 */
static VALUE
rb_ctdb_index_get_key_length(VALUE self)
{
    VRLEN len;
    len = ctdbGetIndexKeyLength(*CTH(self));
    return len == -1 ? Qnil : INT2FIX(len);
}

/*
 * Check if the duplicate flag is set to false.
 */
static VALUE
rb_ctdb_index_is_unique(VALUE self)
{
    return ctdbGetIndexDuplicateFlag(CTH(self)) == YES ? Qfalse : Qtrue;
}

// ===============
// = CT::Segment =
// ===============
void
free_rb_ctdb_segment(struct ctree* ct)
{
    free(ct);
}

VALUE
rb_ctdb_segment_new(VALUE klass, CTHANDLE segment)
{
    struct ctree* ct;

    VALUE obj = Data_Make_Struct(klass, struct ctree, 0, free_rb_ctdb_segment, ct);
    ct->handle = segment;

    rb_obj_call_init(obj, 0, NULL); // CT::Segment.initialize

    return obj;
}

static VALUE
rb_ctdb_segment_get_field(VALUE self)
{
    pCTHANDLE cth = CTH(self);
    CTHANDLE f;

    if((f = ctdbGetSegmentField(*cth)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetSegmentField failed.", ctdbGetError(*cth));

    return rb_ctdb_field_new(cCTField, f);
}

static VALUE
rb_ctdb_segment_set_field(VALUE self, VALUE field)
{
    // pCTHANDLE f = CTH(field);
    return self;
}

static VALUE
rb_ctdb_segment_get_field_name(VALUE self)
{
    pCTHANDLE cth = CTH(self);
    pTEXT name;

    if((name = ctdbGetSegmentFieldName(*cth)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetSegmentFieldName failed.", 
            ctdbGetError(*cth));

    return rb_str_new2(name);
}

static VALUE
rb_ctdb_segment_get_mode(VALUE self)
{
    return self;
}

static VALUE
rb_ctdb_segment_set_mode(VALUE self, VALUE mode)
{
    return self;
}

static VALUE
rb_ctdb_segment_move(VALUE self, VALUE index)
{
    return self;
}

static VALUE
rb_ctdb_segment_get_number(VALUE self)
{
    return self;
}

static VALUE
rb_ctdb_segment_get_status(VALUE self)
{
    return self;
}

// ==============
// = CT::Record =
// ==============
static void
free_rb_ctdb_record(struct ctree_record* ctrec)
{
    ctdbFreeRecord(ctrec->handle);
    free(ctrec);
}

VALUE
rb_ctdb_record_new(VALUE klass, VALUE table)
{
    Check_Type(table, T_DATA);

    struct ctree_record* ctrec;
    VALUE self;

    self = Data_Make_Struct(klass, struct ctree_record, 0, free_rb_ctdb_record, ctrec);
    ctrec->table_ptr = CTH(table);
    if((ctrec->handle = ctdbAllocRecord(*ctrec->table_ptr)) == NULL)
        rb_raise(cCTError, "[%d] ctdbAllocRecord failed.", 
            ctdbGetError(*ctrec->table_ptr));

    VALUE argv[1] = { table };
    rb_obj_call_init(self, 1, argv); // CT::Record.initialize(table)
    return self;
}

static VALUE
rb_ctdb_record_init(VALUE self, VALUE table)
{
    return self;
}

/*
 * Clear the record buffer.
 *
 * @raise [CT::Error] ctdbClearRecord failed.
 */
static VALUE
rb_ctdb_record_clear(VALUE self)
{
    pCTHANDLE cth = CTH(self);

    if(ctdbClearRecord(*cth) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbClearRecord failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retrieves the current default index name. When the record handle is initialized 
 * for the first time, the default index is set to zero.
 *
 * @return [String, nil]
 */
static VALUE
rb_ctdb_record_get_default_index(VALUE self)
{
    CTHANDLE ndx;
    pTEXT name;

    ndx  = ctdbGetDefaultIndexName(*CTRecord(self)->table_ptr);
    name = ctdbGetIndexName(ndx);
    return name ? rb_str_new2(name) : Qnil;
}

/*
 * Retrieves the current filter expression for the record.
 *
 * @return [String, nil] The expression or nil is no filters are active.
 */ 
static VALUE
rb_ctdb_record_get_filter(VALUE self)
{
    pTEXT filter = ctdbGetFilter(*CTRecordH(self));
    return filter ? rb_str_new2(filter) : Qnil;
}

/*
 * Set the filtering logic for the record.
 *
 * @param [String] filter The filter expression.
 * @raise [CT::Error] ctdbFilterRecord failed.
 */
static VALUE
rb_ctdb_record_set_filter(VALUE self, VALUE filter)
{
    Check_Type(filter, T_STRING);

    pCTHANDLE cth = CTRecordH(self);

    if(ctdbFilterRecord(*cth, RSTRING_PTR(filter)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbFilterRecord failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Indicate if the record is being filtered or not.
 */
static VALUE
rb_ctdb_record_is_filtered(VALUE obj)
{
    return ctdbIsFilteredRecord(*CTRecordH(obj)) == YES ? Qtrue : Qfalse;
}

/*
 * Get the first record on a table
 */
static VALUE
rb_ctdb_record_first(VALUE self)
{
    return ctdbFirstRecord(*CTRecordH(self)) == CTDBRET_OK ? self : Qnil;
}

/*
 * @see #first
 * @raise [CT::Error] ctdbFirstRecord failed.
 */
static VALUE
rb_ctdb_record_first_bang(VALUE self)
{
    pCTHANDLE cth = CTRecordH(self);

    if(ctdbFirstRecord(*cth) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbFirstRecord failed.", ctdbGetError(*cth));

    return self;
}

/*
 *
 * 
 * @param [Fixnum] num The field number.
 * @return [Boolean]
 * @raise [CT::Error] ctdbGetFieldAsBool failed.
 */
static VALUE
rb_ctdb_record_get_field_as_bool(VALUE self, VALUE num)
{
    Check_Type(num, T_FIXNUM);

    CTBOOL value;
    pCTHANDLE cth = CTH(self);

    if(ctdbGetFieldAsBool(*cth, FIX2INT(num), &value) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsBool failed.", ctdbGetError(*cth));

    if(value == NULL) return Qnil;

    return value == YES ? Qtrue : Qfalse;
}

/*
 * Retrieve the field as a signed value.
 *
 * @param [Fixnum] num The field number.
 * @return [Fixnum]
 * @raise [CT::Error] ctdbGetFieldAsSigned failed.
 */
static VALUE
rb_ctdb_record_get_field_as_signed(VALUE self, VALUE num)
{
    Check_Type(num, T_FIXNUM);

    CTSIGNED value;
    pCTHANDLE cth = CTH(self);

    if(ctdbGetFieldAsSigned(*cth, FIX2INT(num), &value) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsSigned failed.", ctdbGetError(*cth));

    return INT2FIX(value);
}

/*
 * Retrieve the field as a string value.
 *
 * @param [Fixnum] num The field number.
 * @return [String]
 * @raise [CT::Error] ctdbGetFieldAsString failed.
 */
static VALUE
rb_ctdb_record_get_field_as_string(VALUE self, VALUE num)
{
    Check_Type(num, T_FIXNUM);

    CTSTRING value;
    pCTHANDLE cth = CTH(self);

    // if(ctdbGetFieldAsString(*cth, FIX2INT(num), &value) != CTDBRET_OK)
    //     rb_raise(cCTError, "[%d] ctdbGetFieldAsString failed.", ctdbGetError(*cth));

    return rb_str_new2(value);
}

/*
 *
 *
 * @return [Fixnum]
 * @raise [CT::Error] ctdbGetFieldAsUnsigned failed.
 */
static VALUE
rb_ctdb_record_get_field_as_unsigned(VALUE self, VALUE num)
{
    Check_Type(num, T_FIXNUM);

    pCTHANDLE cth = CTH(self);
    CTUNSIGNED value;

    if(ctdbGetFieldAsUnsigned(*cth, FIX2INT(num), &value) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsUnsigned failed.", ctdbGetError(*cth));

    return UINT2NUM(value);
}

/*
 * Get the last record on a table.
 *
 * @return [CT::Record, nil]
 */
static VALUE
rb_ctdb_record_last(VALUE self)
{
    return ctdbLastRecord(*CTRecordH(self)) == CTDBRET_OK ? self : Qnil;
}

/*
 * @see #last
 * @raise [CT::Error] ctdbLastRecord failed.
 */
static VALUE
rb_ctdb_record_last_bang(VALUE self)
{
    pCTHANDLE cth = CTRecordH(self);

    if(ctdbLastRecord(*cth) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbLastRecord failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Lock the current record.
 *
 * @param [Fixnum] mode The record lock mode.
 */
static VALUE
rb_ctdb_record_lock(VALUE self, VALUE mode)
{
    pCTHANDLE cth = CTRecordH(self);
    return ((ctdbLockRecord(*cth, FIX2INT(mode)) == CTDBRET_OK) ? Qtrue : Qfalse);
}

/*
 * @see #lock
 * @raise [CT::Error] ctdbLockRecord failed.
 */
static VALUE
rb_ctdb_record_lock_bang(VALUE self, VALUE mode)
{
    pCTHANDLE cth = CTRecordH(self);

    if(ctdbLockRecord(*cth, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbLockRecord failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Get the next record on a table.
 *
 * @return [CT::Record, nil]
 */
static VALUE
rb_ctdb_record_next(VALUE self)
{
    return ctdbNextRecord(*CTRecordH(self)) == CTDBRET_OK ? self : Qnil;
}

/*
 * Get the previous record on a table.
 *
 * @return [CT::Record, nil] The previous record or nil if the record does not exist.
 */
static VALUE
rb_ctdb_record_prev(VALUE self)
{
    return ctdbPrevRecord(*CTRecordH(self)) == CTDBRET_OK ? self : Qnil;
}

/*
 * Set the new default index by name or number.
 *
 * @param [String, Symbol, Fixnum] The index identifier.
 * @raise [CT::Error] ctdbSetDefaultIndexByName or ctdbSetDefaultIndex failed.
 */
static VALUE
rb_ctdb_record_set_default_index(VALUE self, VALUE identifier)
{
    pCTHANDLE cth = *CTRecord(self)->table_ptr;
    CTDBRET rc;

    switch(TYPE(identifier)){
        case T_STRING :
        case T_SYMBOL :
            rc = ctdbSetDefaultIndexByName(*cth, RSTRING_PTR(RSTRING(identifier)));
            break;
        case T_FIXNUM :
            rc = ctdbSetDefaultIndex(*cth, FIX2INT(identifier));
            break;
        default :
            rb_raise(rb_eArgError, "Unexpected value type `%s'", 
                     rb_obj_classname(identifier));
            break;
    }

    if(rc != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetDefaultIndex failed.", ctdbGetError(*cth));

    return Qtrue;
}

// static VALUE
// rb_ctdb_record_set_field_as_binary(VALUE self, VALUE num, VALUE value){}

// static VALUE
// rb_ctdb_record_set_field_as_blob(VALUE self, VALUE num, VALUE value){}

/*
 *
 *
 * @param [Fixnum] num Field number
 * @param [Boolean] value
 * @raise [CT::Error] ctdbSetFieldAsBool failed.
 */
static VALUE
rb_ctdb_record_set_field_as_bool(VALUE self, VALUE num, VALUE value)
{
    Check_Type(num, T_FIXNUM);
    if(rb_type(value) != T_TRUE && rb_type(value) != T_FALSE)
        rb_raise(rb_eArgError, "Unexpected value type `%s' for CT_BOOL", 
                 rb_obj_classname(value));

    pCTHANDLE cth = CTRecordH(self);
    CTBOOL cval = (rb_type(value) == T_TRUE ? YES : NO);
    if(ctdbSetFieldAsBool(*cth, FIX2INT(num), cval) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsBool falied.", ctdbGetError(*cth));

    return self;
}

// static VALUE
// rb_ctdb_record_set_field_as_currency(VALUE self, VALUE num, VALUE value){}

static VALUE
rb_ctdb_record_set_field_as_date(VALUE self, VALUE num, VALUE value)
{
    Check_Type(num, T_FIXNUM);

    pCTHANDLE cth = CTRecordH(self);

    if(ctdbSetFieldAsDate(*cth, FIX2INT(num), CTDate(value)->val) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsDate failed.", ctdbGetError(*cth));

    return self;
}

// static VALUE
// rb_ctdb_record_set_field_as_datetime(VALUE self, VALUE num, VALUE value){}

static VALUE
rb_ctdb_record_set_field_as_float(VALUE self, VALUE num, VALUE value)
{
    Check_Type(num, T_FIXNUM);
    Check_Type(value, T_FLOAT);

    pCTHANDLE cth = CTRecordH(self);

    if(ctdbSetFieldAsFloat(*cth, FIX2INT(num), RFLOAT_VALUE(value)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsFloat failed.", ctdbGetError(*cth));

    return self;
}

// static VALUE
// rb_ctdb_record_set_field_as_money(VALUE self, VALUE num, VALUE value){}

// static VALUE
// rb_ctdb_record_set_field_as_number(VALUE self, VALUE num, VALUE value){}

/*
 * Set field as CTSIGNED type value.
 *
 * @param [Fixnum] num Field number
 * @param [Integer] value
 * @raise [CT::Error] ctdbSetFieldAsSigned failed.
 */ 
static VALUE
rb_ctdb_record_set_field_as_signed(VALUE self, VALUE num, VALUE value)
{
    Check_Type(num, T_FIXNUM);
    pCTHANDLE cth = CTRecordH(self);

    if(ctdbSetFieldAsSigned(*cth, FIX2INT(num), 
            (CTSIGNED)FIX2INT(value)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsSigned failed.", ctdbGetError(*cth));

    return self;
}

/*
 *
 *
 * @param [Fixnum] num Field number
 * @param [String] value
 * @raise [CT::Error] ctdbSetFieldAsString failed.
 */
static VALUE
rb_ctdb_record_set_field_as_string(VALUE self, VALUE num, VALUE value)
{
    Check_Type(num, T_FIXNUM);
    Check_Type(value, T_STRING);

    pCTHANDLE cth = CTRecordH(self);

    if(ctdbSetFieldAsString(*cth, FIX2INT(num), RSTRING_PTR(value)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsString failed.", ctdbGetError(*cth));

    return self;
}

// static VALUE
// rb_ctdb_record_set_field_as_time(VALUE self, VALUE num, VALUE value){}

/*
 *
 * Set field as CTUNSIGNED type value.
 *
 * @param [Fixnum] num Field number
 * @param [Integer] value
 * @raise [CT::Error] ctdbSetFieldAsUnsigned failed.
 */
static VALUE
rb_ctdb_record_set_field_as_unsigned(VALUE self, VALUE num, VALUE value)
{
    Check_Type(num, T_FIXNUM);

    pCTHANDLE cth = CTRecordH(self);

    if(ctdbSetFieldAsUnsigned(*cth, FIX2INT(num), 
            (CTUNSIGNED)FIX2INT(value)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsUnsigned failed.", ctdbGetError(*cth));

    return self;
}

// static VALUE
// rb_ctdb_record_set_field_as_utf16(VALUE self, VALUE num, VALUE value){}

/*
 * Create or update an existing record.
 *
 * @raise [CT::Error] ctdbWriteRecord failed.
 */
static VALUE
rb_ctdb_record_write_bang(VALUE self)
{
    pCTHANDLE cth = CTRecordH(self);

    if(ctdbWriteRecord(*cth) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbWriteRecord failed.", ctdbGetError(*cth));

    return self;
}

// ============
// = CT::Date =
// ============
static void
free_rb_ctdb_date(struct ctree_date* ct)
{
    free(ct);
}

/*
 *
 *
 * @param [Fixnum] year
 * @param [Fixnum] month
 * @param [Fixnum] day
 * @raise [CT::Error] ctdbDatePack failed.
 */
VALUE
rb_ctdb_date_new(VALUE klass, VALUE year, VALUE month, VALUE day)
{
    Check_Type(year, T_FIXNUM);
    Check_Type(month, T_FIXNUM);
    Check_Type(day, T_FIXNUM);

    NINT y = FIX2INT(year);
    NINT m = FIX2INT(month);
    NINT d = FIX2INT(day);

    struct ctree_date *ct;
    VALUE obj = Data_Make_Struct(klass, struct ctree_date, 0, free_rb_ctdb_date, ct);

    if(ctdbDatePack(&ct->val, y, m, d) != CTDBRET_OK)
        rb_raise(cCTError, "ctdbDatePack failed.");

    VALUE argv[3] = { year, month, day };
    rb_obj_call_init(obj, 3, argv); // CT::Date.initialize(y,m,d)

    return obj;
}

static VALUE
rb_ctdb_date_init(VALUE self, VALUE year, VALUE month, VALUE day)
{
    return self;
}

/*
 * Retrieve the current date.
 *
 * @return [CT::Date]
 * @raise [CT::Error] ctdbCurrentDate or ctdbDateUnpack failed.
 */
static VALUE
rb_ctdb_date_get_today(VALUE klass)
{
    CTDATE date;

    if(ctdbCurrentDate(&date) != CTDBRET_OK)
        rb_raise(cCTError, "ctdbCurrentDate failed.");

    NINT y, m, d;
    if(ctdbDateUnpack(date, &y, &m, &d) != CTDBRET_OK)
        rb_raise(cCTError, "ctdbDateUnpack failed.");

    return rb_ctdb_date_new(cCTDate, INT2FIX(y), INT2FIX(m), INT2FIX(d));
}

/*
 *
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_date_get_day(VALUE self)
{
    NINT day = ctdbGetDay(CTDate(self)->val);
    return INT2FIX(day);
}

/*
 *
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_date_get_day_of_week(VALUE self)
{
    NINT day = ctdbDayOfWeek(CTDate(self)->val);
    return INT2FIX(day);
}

static VALUE
rb_ctdb_date_is_leap_year(VALUE self)
{
    return ctdbIsLeapYear(CTDate(self)->val) == YES ? Qtrue : Qfalse;
}

/*
 *
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_date_get_month(VALUE self)
{
    NINT month = ctdbGetMonth(CTDate(self)->val);
    return INT2FIX(month);
}

/*
 *
 *
 */
static VALUE
rb_ctdb_date_strftime(VALUE self, VALUE format)
{
    Check_Type(format, T_FIXNUM);

    // pTEXT str;
    // CTDBRET rc;
    // VRLEN buf = 11;
    // 
    // if((rc = ctdbDateToString(CTDate(self)->val, FIX2INT(format), *&str, buf)) != CTDBRET_OK)
    //     rb_raise(cCTError, "[%d] ctdbDateToString failed.", rc);
    // 
    // printf("[%d]\n", __LINE__);
    // 
    // return rb_str_new2(str);
    return self;
}

/*
 *
 *
 */
static VALUE
rb_ctdb_date_strptime(VALUE self, VALUE string, VALUE format)
{
    return self;
}

/*
 *
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_date_get_year(VALUE self)
{
    NINT year = ctdbGetYear(CTDate(self)->val);
    return INT2FIX(year);
}

void 
Init_ctdb_sdk(void)
{
    mCT = rb_define_module("CT");
    // c-treeDB Session Types
    rb_define_const(mCT, "SESSION_CTDB",  INT2FIX(CTSESSION_CTDB));
    rb_define_const(mCT, "SESSION_CTREE", INT2FIX(CTSESSION_CTREE));
    // c-treeDB Find Modes
    rb_define_const(mCT, "FIND_EQ", CTFIND_EQ);
    rb_define_const(mCT, "FIND_LT", CTFIND_LT);
    rb_define_const(mCT, "FIND_LE", CTFIND_LE);
    rb_define_const(mCT, "FIND_GT", CTFIND_GT);
    rb_define_const(mCT, "FIND_GE", CTFIND_GE);
    // c-treeDB Lock Modes
    rb_define_const(mCT, "LOCK_FREE",       CTLOCK_FREE);
    rb_define_const(mCT, "LOCK_READ",       CTLOCK_READ);
    rb_define_const(mCT, "LOCK_READ_BLOCK", CTLOCK_READ_BLOCK);
    rb_define_const(mCT, "LOCK_WRITE",      CTLOCK_WRITE);
    rb_define_const(mCT, "LOCK_WRITE_LOCK", CTLOCK_WRITE_BLOCK);
    // c-treeDB Table create Modes
    rb_define_const(mCT, "CREATE_NORMAL",    INT2FIX(CTCREATE_NORMAL));
    rb_define_const(mCT, "CREATE_PREIMG",    INT2FIX(CTCREATE_PREIMG));
    rb_define_const(mCT, "CREATE_TRNLOG",    INT2FIX(CTCREATE_TRNLOG));
    rb_define_const(mCT, "CREATE_WRITETHRU", INT2FIX(CTCREATE_WRITETHRU));
    rb_define_const(mCT, "CREATE_CHECKLOCK", INT2FIX(CTCREATE_CHECKLOCK));
    rb_define_const(mCT, "CREATE_NORECBYT",  INT2FIX(CTCREATE_NORECBYT));
    rb_define_const(mCT, "CREATE_NOROWID",   INT2FIX(CTCREATE_NOROWID));
    rb_define_const(mCT, "CREATE_CHECKREAD", INT2FIX(CTCREATE_CHECKREAD));
    rb_define_const(mCT, "CREATE_HUGEFILE",  INT2FIX(CTCREATE_HUGEFILE));
    rb_define_const(mCT, "CREATE_NODELFLD",  INT2FIX(CTCREATE_NODELFLD));
    rb_define_const(mCT, "CREATE_NONULFLD",  INT2FIX(CTCREATE_NONULFLD));
    // c-treeDB Table open modes
    rb_define_const(mCT, "OPEN_NORMAL",    INT2FIX(CTOPEN_NORMAL));
    rb_define_const(mCT, "OPEN_DATAONLY",  INT2FIX(CTOPEN_DATAONLY));
    rb_define_const(mCT, "OPEN_EXCLUSIVE", INT2FIX(CTOPEN_EXCLUSIVE));
    rb_define_const(mCT, "OPEN_PERMANENT", INT2FIX(CTOPEN_PERMANENT));
    rb_define_const(mCT, "OPEN_CORRUPT",   INT2FIX(CTOPEN_CORRUPT));
    rb_define_const(mCT, "OPEN_CHECKLOCK", INT2FIX(CTOPEN_CHECKLOCK));
    rb_define_const(mCT, "OPEN_CHECKREAD", INT2FIX(CTOPEN_CHECKREAD));
    rb_define_const(mCT, "OPEN_READONLY",  INT2FIX(CTOPEN_READONLY));
    // c-treeDB Index Types
    rb_define_const(mCT, "INDEX_FIXED",   INT2FIX(CTINDEX_FIXED));
    rb_define_const(mCT, "INDEX_LEADING", CTINDEX_LEADING);
    rb_define_const(mCT, "INDEX_PADDING", CTINDEX_PADDING);
    rb_define_const(mCT, "INDEX_LEADPAD", CTINDEX_LEADPAD);
    rb_define_const(mCT, "INDEX_ERROR",   CTINDEX_ERROR);
    // c-treeDB Field Types
    rb_define_const(mCT, "BOOL",      INT2FIX(CT_BOOL));
    rb_define_const(mCT, "TINYINT",   INT2FIX(CT_TINYINT));
    rb_define_const(mCT, "UTINYINT",  INT2FIX(CT_UTINYINT));
    rb_define_const(mCT, "SMALLINT",  INT2FIX(CT_SMALLINT));
    rb_define_const(mCT, "USMALLINT", INT2FIX(CT_USMALLINT));
    rb_define_const(mCT, "INTEGER",   INT2FIX(CT_INTEGER));
    rb_define_const(mCT, "UINTEGER",  INT2FIX(CT_UINTEGER));
    rb_define_const(mCT, "MONEY",     INT2FIX(CT_MONEY));
    rb_define_const(mCT, "DATE",      INT2FIX(CT_DATE));
    rb_define_const(mCT, "TIME",      INT2FIX(CT_TIME));
    rb_define_const(mCT, "FLOAT",     INT2FIX(CT_FLOAT));
    rb_define_const(mCT, "DOUBLE",    INT2FIX(CT_DOUBLE));
    rb_define_const(mCT, "TIMESTAMP", INT2FIX(CT_TIMESTAMP));
    rb_define_const(mCT, "EFLOAT",    INT2FIX(CT_EFLOAT));
    rb_define_const(mCT, "BINARY",    INT2FIX(CT_BINARY));
    rb_define_const(mCT, "CHARS",     INT2FIX(CT_CHARS));
    rb_define_const(mCT, "FPSTRING",  INT2FIX(CT_FPSTRING));
    rb_define_const(mCT, "F2STRING",  INT2FIX(CT_F2STRING));
    rb_define_const(mCT, "F4STRING",  INT2FIX(CT_F4STRING));
    rb_define_const(mCT, "BIGINT",    INT2FIX(CT_BIGINT));
    rb_define_const(mCT, "NUMBER",    INT2FIX(CT_NUMBER));
    rb_define_const(mCT, "CURRENCY",  INT2FIX(CT_CURRENCY));
    rb_define_const(mCT, "PSTRING",   INT2FIX(CT_PSTRING));
    rb_define_const(mCT, "VARBINARY", INT2FIX(CT_VARBINARY));
    rb_define_const(mCT, "LVB",       INT2FIX(CT_LVB));
    rb_define_const(mCT, "VARCHAR",   INT2FIX(CT_VARCHAR));
    rb_define_const(mCT, "LVC",       INT2FIX(CT_LVC));
    // c-ctreeDB Table alter modes
    rb_define_const(mCT, "DB_ALTER_NORMAL",   CTDB_ALTER_NORMAL);
    rb_define_const(mCT, "DB_ALTER_INDEX",    CTDB_ALTER_INDEX);
    rb_define_const(mCT, "DB_ALTER_FULL",     CTDB_ALTER_FULL);
    rb_define_const(mCT, "DB_ALTER_PURGEDUP", CTDB_ALTER_PURGEDUP);
    // c-treeDB Table status
    rb_define_const(mCT, "DB_REBUILD_NONE",     CTDB_REBUILD_NONE);
    rb_define_const(mCT, "DB_REBUILD_DODA",     CTDB_REBUILD_DODA);
    rb_define_const(mCT, "DB_REBUILD_RESOURCE", CTDB_REBUILD_RESOURCE);
    rb_define_const(mCT, "DB_REBUILD_INDEX",    CTDB_REBUILD_INDEX);
    rb_define_const(mCT, "DB_REBUILD_ALL",      CTDB_REBUILD_ALL);
    rb_define_const(mCT, "DB_REBUILD_FULL",     CTDB_REBUILD_FULL);
    // c-ctreeDB Date type
    rb_define_const(mCT, "DATE_MDCY", INT2FIX(CTDATE_MDCY)); // mm/dd/ccyy
    rb_define_const(mCT, "DATE_MDY",  CTDATE_DMCY); // mm/dd/yy
    rb_define_const(mCT, "DATE_DMCY", CTDATE_DMCY); // dd/mm/ccyy
    rb_define_const(mCT, "DATE_DMY",  CTDATE_DMY);  // dd/mm/yy
    rb_define_const(mCT, "DATE_CYMD", CTDATE_CYMD); // ccyymmdd
    rb_define_const(mCT, "DATE_YMD",  CTDATE_YMD);  // yymmdd

    // c-ctreeDB Date regular expressions
    const char* date_mdcy_regex = "^ *(0[1-9]|1[0-2])/\\d{2}/\\d{4}$";
    rb_define_const(mCT, "DATE_MDCY_REGEX", 
                    rb_reg_new(date_mdcy_regex, strlen(date_mdcy_regex), 0));

    const char* date_mdy_regex = "^ *(0[1-9]|1[0-2])(/|-)((0[1-9])|((1|2)[0-9])|(3[0-1]))(/|-)[0-9]{2} *$";
    rb_define_const(mCT, "DATE_MDY_REGEX", 
                    rb_reg_new(date_mdy_regex, strlen(date_mdy_regex), 0));

    const char* date_dmcy_regex = "^ *(0[1-3]|[1-3][0-9])/(0[1-9]|1[0-2])/[1-2]\\d{2}$";
    rb_define_const(mCT, "DATE_DMCY_REGEX", 
                    rb_reg_new(date_dmcy_regex, strlen(date_dmcy_regex), 0));

    const char* date_dmy_regex = "^[0-3][0-9]/(0[1-9]|1[0-2])/\\d{2}$";
    rb_define_const(mCT, "DATE_DMY_REGEX",
                    rb_reg_new(date_dmy_regex, strlen(date_dmy_regex), 0));

    const char* date_cymd_regex = "^\\d{8}$"; 
    rb_define_const(mCT, "DATE_CYMD_REGEX",
                    rb_reg_new(date_cymd_regex, strlen(date_cymd_regex), 0));

    const char* date_ymd_regex = "^[0-9]{6}$";
    rb_define_const(mCT, "DATE_YMD_REGEX",
                    rb_reg_new(date_ymd_regex, strlen(date_ymd_regex), 0));

    // CT::Error
    cCTError = rb_define_class_under(mCT, "Error", rb_eStandardError);

    // CT::Session
    cCTSession = rb_define_class_under(mCT, "Session", rb_cObject);
    rb_define_singleton_method(cCTSession, "new", rb_ctdb_session_new, 1);
    rb_define_method(cCTSession, "active?", rb_ctdb_session_is_active, 0);
    rb_define_method(cCTSession, "default_date_type", rb_ctdb_get_defualt_date_type, 0);
    rb_define_method(cCTSession, "default_date_type=", rb_ctdb_set_defualt_date_type, 1);
    // rb_define_method(cCTSession, "delete_table", rb_ctdb_session_delete_table, 1);
    rb_define_method(cCTSession, "lock", rb_ctdb_session_lock, 1);
    rb_define_method(cCTSession, "lock!", rb_ctdb_session_lock_bang, 1);
    rb_define_method(cCTSession, "locked?", rb_ctdb_session_is_locked, 0);
    rb_define_method(cCTSession, "logon", rb_ctdb_session_logon, -1);
    rb_define_method(cCTSession, "logout", rb_ctdb_session_logout, 0);
    rb_define_method(cCTSession, "password", rb_ctdb_session_get_password, 0);
    rb_define_method(cCTSession, "path_prefix", rb_ctdb_session_get_path_prefix, 0);
    rb_define_method(cCTSession, "path_prefix=", rb_ctdb_session_set_path_prefix, 1);
    rb_define_method(cCTSession, "unlock", rb_ctdb_session_unlock, 0);
    rb_define_method(cCTSession, "unlock!", rb_ctdb_session_unlock_bang, 0);
    rb_define_method(cCTSession, "username", rb_ctdb_session_get_username, 0);

    // CT::Table
    cCTTable = rb_define_class_under(mCT, "Table", rb_cObject);
    rb_define_singleton_method(cCTTable, "new", rb_ctdb_table_new, 1);
    rb_define_method(cCTTable, "initialize", rb_ctdb_table_init, 1);
    rb_define_method(cCTTable, "add_field", rb_ctdb_table_add_field, 3);
    rb_define_method(cCTTable, "add_index", rb_ctdb_table_add_index, 2);
    rb_define_method(cCTTable, "alter", rb_ctdb_table_alter, 1);
    rb_define_method(cCTTable, "close", rb_ctdb_table_close, 0);
    rb_define_method(cCTTable, "create", rb_ctdb_table_create, 2);
    rb_define_method(cCTSession, "default_date_type", rb_ctdb_get_defualt_date_type, 0);
    rb_define_method(cCTSession, "default_date_type=", rb_ctdb_set_defualt_date_type, 1);
    rb_define_method(cCTTable, "delim_char", rb_ctdb_table_get_delim_char, 0);

    rb_define_method(cCTTable, "field_count", rb_ctdb_table_get_field_count, 0);
    rb_define_method(cCTTable, "field_names", rb_ctdb_table_get_field_names, 0);
    rb_define_method(cCTTable, "get_field", rb_ctdb_table_get_field, 1);
    rb_define_method(cCTTable, "get_fields", rb_ctdb_table_get_fields, 0);
    rb_define_method(cCTTable, "get_field_by_name", rb_ctdb_table_get_field_by_name, 1);
    // rb_define_method(cCTTable, "indecies", rb_ctdb_table_get_indecies, 0);
    // rb_define_method(cCTTable, "index", rb_ctdb_table_get_index, 1);
    rb_define_method(cCTTable, "name", rb_ctdb_table_get_name, 0);
    rb_define_method(cCTTable, "open", rb_ctdb_table_open, 2);
    rb_define_method(cCTTable, "pad_char", rb_ctdb_table_get_pad_char, 0);
    rb_define_method(cCTTable, "path", rb_ctdb_table_get_path, 0);
    rb_define_method(cCTTable, "path=", rb_ctdb_table_set_path, 1);
    rb_define_method(cCTTable, "status", rb_ctdb_table_get_status, 0);

    // CT::Field
    cCTField = rb_define_class_under(mCT, "Field", rb_cObject);
    // rb_define_singleton_method(cCTField, "new", rb_ctdb_field_new, -1);
    rb_define_method(cCTField, "allow_nil?", rb_ctdb_field_get_null_flag, 0);
    rb_define_method(cCTField, "default", rb_ctdb_field_get_default, 0);
    rb_define_method(cCTField, "default=", rb_ctdb_field_set_default, 1);
    rb_define_method(cCTField, "length", rb_ctdb_field_get_length, 0);
    rb_define_method(cCTField, "length=", rb_ctdb_field_set_length, 1);
    rb_define_method(cCTField, "name", rb_ctdb_field_get_name, 0);
    rb_define_method(cCTField, "name=", rb_ctdb_field_set_name, 1);
    rb_define_method(cCTField, "precision", rb_ctdb_field_get_precision, 0);
    rb_define_method(cCTField, "precision=", rb_ctdb_field_set_precision, 1);
    rb_define_method(cCTField, "scale", rb_ctdb_field_get_scale, 0);
    rb_define_method(cCTField, "scale=", rb_ctdb_field_set_scale, 1);
    rb_define_method(cCTField, "type", rb_ctdb_field_get_type, 0);
    rb_define_method(cCTField, "type=", rb_ctdb_field_set_type, 1);

    // CT::Index
    cCTIndex = rb_define_class_under(mCT, "Index", rb_cObject);
    // rb_define_singleton_method(cCTTable, "new", rb_ctdb_index_init, 4);
    rb_define_method(cCTIndex, "add_segment", rb_ctdb_index_add_segment, 2);
    rb_define_method(cCTIndex, "key_length", rb_ctdb_index_get_key_length, 0);
    rb_define_method(cCTIndex, "key_type",   rb_ctdb_index_get_key_type, 0);
    rb_define_method(cCTIndex, "name", rb_ctdb_index_get_name, 0);
    // rb_define_method(cCTIndex, "number", rb_ctdb_index_get_number, 0);
    rb_define_method(cCTIndex, "unique?", rb_ctdb_index_is_unique, 0);

    // CT::Segment
    cCTSegment = rb_define_class_under(mCT, "Segment", rb_cObject);
    rb_define_singleton_method(cCTSegment, "new", rb_ctdb_segment_new, 1);
    rb_define_method(cCTSegment, "field_name", rb_ctdb_segment_get_field_name, 0);
    rb_define_method(cCTSegment, "field", rb_ctdb_segment_get_field, 0);
    rb_define_method(cCTSegment, "field=", rb_ctdb_segment_set_field, 1);
    rb_define_method(cCTSegment, "mode", rb_ctdb_segment_get_mode, 0);
    rb_define_method(cCTSegment, "mode=", rb_ctdb_segment_set_mode, 1);
    rb_define_method(cCTSegment, "move", rb_ctdb_segment_move, 1);
    rb_define_method(cCTSegment, "number", rb_ctdb_segment_get_number, 1);
    rb_define_method(cCTSegment, "status", rb_ctdb_segment_get_status, 0);

    // CT::Record
    cCTRecord = rb_define_class_under(mCT, "Record", rb_cObject);
    rb_define_singleton_method(cCTRecord, "new", rb_ctdb_record_new, 1);
    rb_define_method(cCTRecord, "initialize", rb_ctdb_record_init, 1);
    rb_define_method(cCTRecord, "clear", rb_ctdb_record_clear, 0);
    rb_define_method(cCTRecord, "default_index", rb_ctdb_record_get_default_index, 0);
    rb_define_method(cCTRecord, "default_index=", rb_ctdb_record_set_default_index, 1);
    rb_define_method(cCTRecord, "filter", rb_ctdb_record_get_filter, 0);
    rb_define_method(cCTRecord, "filter=", rb_ctdb_record_set_filter, 1);
    rb_define_method(cCTRecord, "filtered?", rb_ctdb_record_is_filtered, 0);
    rb_define_method(cCTRecord, "first", rb_ctdb_record_first, 0);
    rb_define_method(cCTRecord, "first!", rb_ctdb_record_first_bang, 0);
    rb_define_method(cCTRecord, "get_field_as_bool", rb_ctdb_record_get_field_as_bool, 1);
    rb_define_method(cCTRecord, "get_field_as_signed", rb_ctdb_record_get_field_as_signed, 1);
    rb_define_method(cCTRecord, "get_field_as_string", rb_ctdb_record_get_field_as_string, 1);
    rb_define_method(cCTRecord, "get_field_as_unsigned", rb_ctdb_record_get_field_as_unsigned, 1);
    rb_define_method(cCTRecord, "last", rb_ctdb_record_last, 0);
    rb_define_method(cCTRecord, "last!", rb_ctdb_record_last_bang, 0);
    rb_define_method(cCTRecord, "lock", rb_ctdb_record_lock, 1);
    rb_define_method(cCTRecord, "lock!", rb_ctdb_record_lock_bang, 1);
    rb_define_method(cCTRecord, "next", rb_ctdb_record_next, 0);
    rb_define_method(cCTRecord, "prev", rb_ctdb_record_prev, 0);
    // rb_define_method(cCTRecord, "set_field", rb_ctdb_record_set_field, 2);
    rb_define_method(cCTRecord, "set_field_as_bool", rb_ctdb_record_set_field_as_bool, 2);
    rb_define_method(cCTRecord, "set_field_as_date", rb_ctdb_record_set_field_as_date, 2);
    rb_define_method(cCTRecord, "set_field_as_signed", rb_ctdb_record_set_field_as_signed, 2);
    rb_define_method(cCTRecord, "set_field_as_string", rb_ctdb_record_set_field_as_string, 2);
    rb_define_method(cCTRecord, "set_field_as_unsigned", rb_ctdb_record_set_field_as_unsigned, 2);
    rb_define_method(cCTRecord, "write!", rb_ctdb_record_write_bang, 0);

    // CT::Date
    cCTDate = rb_define_class_under(mCT, "Date", rb_cObject);
    rb_define_singleton_method(cCTDate, "new", rb_ctdb_date_new, 3);
    rb_define_singleton_method(cCTDate, "today", rb_ctdb_date_get_today, 0);
    rb_define_method(cCTDate, "initialize", rb_ctdb_date_init, 3);
    rb_define_method(cCTDate, "day", rb_ctdb_date_get_day, 0);
    rb_define_method(cCTDate, "day_of_week", rb_ctdb_date_get_day_of_week, 0);
    rb_define_method(cCTDate, "leap_year?", rb_ctdb_date_is_leap_year, 0);
    rb_define_method(cCTDate, "month", rb_ctdb_date_get_month, 0);
    rb_define_method(cCTDate, "strftime", rb_ctdb_date_strftime, 1);
    rb_define_method(cCTDate, "strptime", rb_ctdb_date_strptime, 2);
    rb_define_method(cCTDate, "year", rb_ctdb_date_get_year, 0);
}