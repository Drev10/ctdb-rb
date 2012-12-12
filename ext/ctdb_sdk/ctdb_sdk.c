#include "ruby.h"
#include "ctdbsdk.h" /* c-tree headers */

#define RUBY_CLASS(name) rb_const_get(rb_cObject, rb_intern(name))

#define CTH(obj) \
    (Check_Type(obj, T_DATA), &(((struct ctree*)DATA_PTR(obj))->handle))
#define CTRecord(obj) \
    (Check_Type(obj, T_DATA), (struct ctree_record*)DATA_PTR(obj))
#define CTRecordH(obj) \
    (Check_Type(obj, T_DATA), &(((struct ctree_record*)DATA_PTR(obj))->handle))

/*
 * Generic structure for storing c-tree resources
 */
struct ctree {
    CTHANDLE handle;
};
/*
 * Custom container for CT::Record resources.
 */
struct ctree_record {
    CTHANDLE handle;
    pCTHANDLE table_ptr;
};

VALUE mCT;
VALUE cCTError;    // CT::Error
VALUE cCTSession;  // CT::Session
VALUE cCTTable;    // CT::Table
VALUE cCTIndex;    // CT::Index
VALUE cCTSegment;  // CT::Segment
VALUE cCTRecord;   // CT::Record
VALUE cCTField;    // CT::Field

// TODO: #define rb_define_ct_const(s, c) rb_define_const(mCT, #s, INT2NUM(c))

// TODO: ctdb.h jazz
VALUE rb_ctdb_field_new(VALUE klass, CTHANDLE field);
VALUE rb_ctdb_index_new(VALUE klass, CTHANDLE index);
VALUE rb_ctdb_segment_new(VALUE klass, CTHANDLE segment);

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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(type, T_FIXNUM);

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
    struct ctree* ct;
    VALUE obj = Data_Make_Struct(klass, struct ctree, 0, free_rb_ctdb_session, ct);
    
    Check_Type(mode, T_FIXNUM);

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
 *
 *
 */
static VALUE
rb_ctdb_session_find_active_table(VALUE self, VALUE name)
{
    Check_Type(name, T_STRING);
    // TODO: rb_ctdb_session_find_active_table
    return self;
}

/*
 *static VALUE
 *rb_ctdb_session_find_table(VALUE self, VALUE name)
 *{
 *    Check_Type(name, T_STRING);
 *
 *    pTEXT n, p;
 *    struct ctree* ct;
 *    
 *    if(ctdbFindTable(*CTH(self), &n, &p) != CTDBRET_OK)
 *        return Qnil;
 *
 *    // TODO: DRY up creating table objects via c
 *
 *    
 *}
 */

/*
 *static VALUE
 *rb_ctdb_session_find_table!(VALUE self, value name)
 *{
 *    Check_Type(name, T_STRING);
 *
 *    pCTHANDLE cth = CTH(self);
 *
 *    if(ctdbFindTable(*cth, ) != CTDBRET_OK)
 *        rb_raise(cCTError, "[%d] ctdbFindTable failed.", ctdbGetError(*cth));
 *        
 *    
 *}
 */

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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(mode, T_FIXNUM);

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
rb_ctdb_session_logon(VALUE self, VALUE engine, VALUE user, VALUE password)
// rb_ctdb_session_logon(int argc, VALUE *argv, VALUE self)
{
    pCTHANDLE cth = CTH(self);
    pTEXT e, u, p;
    
    Check_Type(engine, T_STRING);
    Check_Type(user, T_STRING);
    Check_Type(user, T_STRING);

    e = RSTRING_PTR(engine);
    u = RSTRING_PTR(user);
    p = RSTRING_PTR(password);

    if(ctdbLogon(*cth, e, u, p) != CTDBRET_OK)
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

    return rb_str_new_cstr(pword);
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
    return prefix ? rb_str_new_cstr(prefix) : Qnil;
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
    pCTHANDLE cth = CTH(self);

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

    return rb_str_new_cstr(name);
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
    struct ctree* ct;
    pCTHANDLE cth;
    VALUE obj;
    
    Check_Type(session, T_DATA);

    cth = CTH(session);
    obj = Data_Make_Struct(klass, struct ctree, 0, free_rb_ctdb_table, ct);

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
 * @options opts [Boolean] :allow_nil Set the field null flag.
 * @options opts [Object] :default Set the fields default value.
 * @options opts [Fixnum] :scale The number of digits to the right of the 
 * decimal point.
 * @options opts [Fixnum] :precision The maximun number of digits.
 * @raise [CT::Error]
 */
static VALUE
rb_ctdb_table_add_field(VALUE self, VALUE name, VALUE type, VALUE length)
{
    CTHANDLE field;
    pCTHANDLE cth = CTH(self);
    
    Check_Type(name, T_STRING);
    Check_Type(type, T_FIXNUM);
    Check_Type(length, T_FIXNUM);

    field = ctdbAddField(*cth, RSTRING_PTR(name), FIX2INT(type), FIX2INT(length));
    if(!field) rb_raise(cCTError, "[%d] ctdbAddField failed.", ctdbGetError(*cth));

    return rb_ctdb_field_new(cCTField, field);
}

/*
 * Add a new index the the table.
 *
 * @param [String] name Index name
 * @param [Fixnum] type Key type
 * @param [Hash] opts
 * @options [Boolean] :allow_dups Indication if the index allows duplicate keys.
 * @options [Boolean] :allow_nil Indidication if the index allows null keys.
 * @raise [CT::Error] ctdbAddIndex failed.
 */
static VALUE
rb_ctdb_table_add_index(VALUE self, VALUE name, VALUE type)/*, VALUE opts)*/
{
    // Check_Type(type, T_DATA);
    // Check_Type(opts, T_HASH);

    // if(rb_type(allow_dups) != T_TRUE && rb_type(allow_dups) != T_FALSE)
    //     rb_raise(rb_eArgError, "Unexpected value type `%s' for allow_dups", 
    //              rb_obj_classname(allow_dups));

    // if(rb_type(allow_nil) != T_TRUE && rb_type(allow_nil) != T_FALSE)
    //     rb_raise(rb_eArgError, "Unexpected value type `%s' for allow_nil", 
    //             rb_obj_classname(allow_nil));

    CTHANDLE index;
    pCTHANDLE cth = CTH(self);
    // CTBOOL dflag  = (rb_type(allow_dups) == T_TRUE ? YES : NO);
    // CTBOOL nflag  = (rb_type(allow_nil) == T_TRUE ? YES : NO);
    CTBOOL dflag = NO;
    CTBOOL nflag = YES;
    VALUE ct_index;

    Check_Type(name, T_STRING);

    index = ctdbAddIndex(*cth, RSTRING_PTR(name), FIX2INT(type), dflag, nflag);
    if(!index) rb_raise(cCTError, "[%d] ctdbAddIndex failed.", ctdbGetError(*cth));

    ct_index = rb_ctdb_index_new(cCTIndex, index);

    return ct_index;
}

/*
 * Retrieve an Index by name or number.
 *
 * @param [Fixnum, String] value Field identifier
 * @return [CT::Index]
 */
static VALUE
rb_ctdb_table_get_index(VALUE self, VALUE value)
{
    pCTHANDLE cth = CTH(self);
    CTHANDLE index;

    switch(rb_type(value)){
        case T_STRING :
            index = ctdbGetIndexByName(*cth, RSTRING_PTR(value));
            break;
        case T_FIXNUM :
            index = ctdbGetIndex(*cth, FIX2INT(value));
            break;
        default :
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(value));
            break;
    }

    if(!index)
        rb_raise(cCTError, "[%d] ctdbGetIndex failed.", ctdbGetError(*cth));

    return rb_ctdb_index_new(cCTIndex, index);
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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(mode, T_FIXNUM);

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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(name, T_STRING);
    Check_Type(mode, T_FIXNUM);

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
    TEXT dchar;

    if(ctdbGetPadChar(*cth, NULL, &dchar) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetPadChar failed.", ctdbGetError(*cth));

    return rb_str_new_cstr(&dchar);
}

/*
 * Retrieve a table field based on the field number or name.
 *
 * @param [Fixnum, String] value The field number or name.
 * @return [CT::Field]
 * @raise [CT::Error] ctdbGetField failed.
 */
static VALUE
rb_ctdb_table_get_field(VALUE self, VALUE value)
{
    pCTHANDLE cth = CTH(self);
    CTHANDLE field;

    switch(rb_type(value)){
        case T_STRING :
            field = ctdbGetFieldByName(*cth, (pTEXT)RSTRING_PTR(value));
            break;
        case T_FIXNUM :
            field = ctdbGetField(*cth, FIX2INT(value));
            break;
        default :
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(value));
            break;
    }

    if(!field)
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
    VALUE fields, ct_field;

    if((n = ctdbGetTableFieldCount(*cth)) == -1)
        rb_raise(cCTError, "[%d] ctdbGetTableFieldCount failed.", 
            ctdbGetError(*cth));

    fields = rb_ary_new2(n);

    for(i = 0; i < n; i++){
        // Get the field handle.
        if((field = ctdbGetField(*cth, i)) == NULL)
            rb_raise(cCTError, "[%d] ctdbGetField failed.", ctdbGetError(*cth));

        ct_field = rb_ctdb_field_new(cCTField, field);
        if(rb_block_given_p()) rb_yield(ct_field);
            rb_ary_store(fields, i, ct_field);
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
    pCTHANDLE cth = CTH(self);
    CTHANDLE field;
    
    Check_Type(name, T_STRING);
 
    if(!(field = ctdbGetFieldByName(*cth, RSTRING_PTR(name))))
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
        rb_ary_store(fields, i, rb_str_new_cstr(fname));
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
    VALUE s = rb_str_new_cstr(ctdbGetTableName(*CTH(self)));
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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(name, T_STRING);

    // if(ctdbOpenTable(*cth, RSTRING_PTR(name), FIX2INT(mode)) !=  CTDBRET_OK)
    if(ctdbOpenTable(*cth, RSTRING_PTR(name), CTOPEN_NORMAL) !=  CTDBRET_OK)
        rb_raise(cCTError, "[%d][%d] ctdbOpenTable failed.", 
                ctdbGetError(*cth), sysiocod);

    return self;
}

/*
 * Retrieve the active state of a table.  A table is active if it is open.
 */
static VALUE
rb_ctdb_table_is_active(VALUE self)
{
    return ctdbIsActiveTable(*CTH(self)) == YES ? Qtrue : Qfalse;
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
    TEXT pchar;

    if(ctdbGetPadChar(*cth, &pchar, NULL) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetPadChar failed.", ctdbGetError(*cth));

    return rb_str_new_cstr(&pchar);
}

/*
 * Set the table pad character.
 *
 * @param [String] pad_char The character to pad fixed length fields with.
 */
static VALUE
rb_ctdb_table_set_pad_char(VALUE self, VALUE pad_char)
{
    Check_Type(pad_char, T_STRING);

    // if(ctdbGetPadChar(*cth, NULL, ))
    // 
    // pCTHANDLE cth = CTH(self);
    return self;
}

/*
 * Retrieve the table path.
 *
 * @return [String, nil] The path or nil if one is not set.
 */
static VALUE
rb_ctdb_table_get_path(VALUE self)
{
    VALUE s = rb_str_new_cstr(ctdbGetTablePath(*CTH(self)));
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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(path, T_STRING);

    if(ctdbSetTablePath(*cth, RSTRING_PTR(path)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetTablePath failed.", ctdbGetError(*cth));

    return self;
}

/*
 *
 *
 * @param [Fixnum] mode The rebuild mode.
 */
static VALUE
rb_ctdb_table_rebuild(VALUE self, VALUE mode)
{
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

static VALUE
rb_ctdb_field_inspect(VALUE self)
{
    // pCTHANDLE cth = CTH(self);
    // pTEXT name;
    // 
    // if((name = ctdbGetFieldName(*cth)) == NULL)
    //     rb_raise(cCTError, "[%d] ctdbGetFieldName failed.", ctdbGetError(*cth));
    // 
    // char *n;
    // 
    // sprintf(, "#<CT::Field:%s>", *name);

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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(length, T_FIXNUM);

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
    pTEXT name;
    pCTHANDLE cth = CTH(self);

    if((name = ctdbGetFieldName(*cth)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetFieldName failed.", ctdbGetError(*cth));

    return rb_str_new_cstr(name);
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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(name, T_STRING);

    if(ctdbSetFieldName(*cth, RSTRING_PTR(name)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldName failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retreive the field number in the table field list.
 *
 * @return [Fixnum]
 * @raise [CT::Error] ctdbGetFieldnbr failed.
 */
static VALUE
rb_ctdb_field_get_number(VALUE self)
{
    NINT n;
    pCTHANDLE cth = CTH(self);

    // if((n = ctdbGetFieldNbr(*cth)) == -1)
    //     rb_raise(cCTError, "[%d] ctdbGetFieldNbr failed.", ctdbGetError(*cth));
    n = ctdbGetFieldNbr(*cth);
    return INT2FIX(n);
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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(precision, T_FIXNUM);

    if(ctdbSetFieldPrecision(*cth, FIX2INT(precision)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldPrecision failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Retrieve field properties such as name, type and length.
 *
 * @return [Hash]
 * @raise [CT::Error] ctdbGetFieldProperties failed.
 */
static VALUE
rb_ctdb_field_get_properties(VALUE self)
{
    // pCTHANDLE cth = CTH(self);
    // pTEXT n;
    // pCTDBTYPE t;
    // pVRLEN l;
    // printf("[%d]\n", __LINE__);
    // 
    // if(ctdbGetFieldProperties(*cth, &n, t, l) != CTDBRET_OK)
    //     rb_raise(cCTError, "[%d] ctdbGetFieldProperties failed.", ctdbGetError(*cth));
    // printf("[%d]\n", __LINE__);
    VALUE hash = rb_hash_new();
    // rb_hash_aset(hash, rb_str_new_cstr("name"), rb_str_new_cstr(*n));
    // rb_hash_aset(hash, rb_str_new_cstr("type"), FIX2INT(*t));
    // rb_hash_aset(hash, rb_str_new_cstr("length"), FIX2INT(*l));
    // printf("[%d]\n", __LINE__);
    return hash;
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
    // TODO: rb_ctdb_field_set_scale
    return self;
}

static VALUE
rb_ctdb_field_get_type(VALUE self)
{ 
    CTDBTYPE type;
    pCTHANDLE cth = CTH(self);

    if((type = ctdbGetFieldType(*cth)) == 0)
        rb_raise(cCTError, "[%d] ctdbGetFieldType failed.", ctdbGetError(*cth));

    return INT2FIX(type);
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
    pCTHANDLE cth = CTH(self);
    
    Check_Type(field, T_DATA);

    if(!ctdbAddSegment(*cth, *CTH(field), CTSEG_SCHSEG))
        rb_raise(cCTError, "[%d] ctdbAddSegment failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Check if the duplicate flag is set to false.
 */
static VALUE
rb_ctdb_index_get_allow_dups(VALUE self)
{
    return ctdbGetIndexDuplicateFlag(CTH(self)) == YES ? Qfalse : Qtrue;
}

/*
 * Set the allow duplicate flag for this index.
 *
 * @param [Boolean] value
 * @raise [CT::Error] ctdbSetIndexDuplicateFlag failed.
 */
static VALUE
rb_ctdb_index_set_allow_dups(VALUE self, VALUE value)
{
    pCTHANDLE cth;
    CTBOOL v;
  
    if(rb_type(value) != T_TRUE && rb_type(value) != T_FALSE)
        rb_raise(rb_eArgError, "Unexpected value type `%s' for CT_BOOL", 
                 rb_obj_classname(value));

    cth = CTH(self);
    v = (value == T_TRUE ? YES : NO);

    if(ctdbSetIndexDuplicateFlag(*cth, v) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetIndexDuplicateFlag failed.", 
            ctdbGetError(*cth));

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
    return name ? rb_str_new_cstr(name) : Qnil;
}

/*
 * Retrieve an Array of all Index Segments.
 * @return [Array]
 */
static VALUE
rb_ctdb_index_get_segments(VALUE self)
{
    pCTHANDLE cth;
    VRLEN cnt;
    int j;
    VALUE segments;

    cth = CTH(self);
    segments = rb_ary_new();

    if((cnt = ctdbGetIndexSegmentCount(*cth)) == -1)
        rb_raise(cCTError, "[%d] ctdbGetIndexSegmentCount failed.", 
                ctdbGetError(*cth), cnt);

    for(j = 0; j < cnt; j++)
        rb_ary_push(segments, 
                rb_ctdb_segment_new(cCTSegment, ctdbGetSegment(*cth, j))); 
    
    return segments;
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
    CTHANDLE f;
    pCTHANDLE cth = CTH(self);

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
    pTEXT name;
    pCTHANDLE cth = CTH(self);

    if((name = ctdbGetSegmentFieldName(*cth)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetSegmentFieldName failed.", 
            ctdbGetError(*cth));

    return rb_str_new_cstr(name);
}

static VALUE
rb_ctdb_segment_get_mode(VALUE self)
{
    return INT2FIX(ctdbGetSegmentMode(CTH(self)));
}

static VALUE
rb_ctdb_segment_set_mode(VALUE self, VALUE mode)
{
    // TODO: rb_ctdb_segment_set_mode
    return self;
}

static VALUE
rb_ctdb_segment_move(VALUE self, VALUE index)
{
    // TODO: rb_ctdb_segment_move
    return self;
}

static VALUE
rb_ctdb_segment_get_number(VALUE self)
{
    // TODO: rb_ctdb_segment_get_number
    return self;
}

static VALUE
rb_ctdb_segment_get_status(VALUE self)
{
    // TODO: rb_ctdb_segment_get_status
    return self;
}

/*
 * Hack to identify old school Segments
 */
static VALUE
rb_ctdb_segment_absolute_byte_offset(VALUE self)
{
    CTSEG_MODE mode;
    
    mode = ctdbGetSegmentMode(CTH(self));
    if(mode == CTSEG_REGSEG || mode == CTSEG_UREGSEG || CTSEG_INTSEG ||
            mode == CTSEG_SGNSEG || mode == CTSEG_FLTSEG)
        return Qtrue;
    else
        return Qfalse;
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

/*
 * Helper function for get_field_as_... calls.
 */
CTBOOL
ctdb_record_is_field_null(pCTHANDLE record_ptr, NINT field_num)
{
    return ctdbIsNullField(*record_ptr, field_num);
}

VALUE
rb_ctdb_record_new(VALUE klass, VALUE table)
{
    struct ctree_record* ctrec;
    VALUE self;
    
    Check_Type(table, T_DATA);

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
 * Retrieve the number of records in the table.
 *
 * @return [Fixnum]
 * @raise [CT::Error] ctdbGetRecordCount failed.
 */
static VALUE
rb_ctdb_record_get_count(VALUE self)
{
    CTUINT64 cnt;
    pCTHANDLE cth = CTH(self);

    if(ctdbGetRecordCount(*cth, &cnt) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetRecordCount failed.", ctdbGetError(*cth));



    return INT2FIX(cnt);
}

/*
 * Retrieves the current default index name. When the record handle is initialized 
 * for the first time, the default index is set to zero.
 *
 * @return [CT::Index, nil]
 */
static VALUE
rb_ctdb_record_get_default_index(VALUE self)
{
    NINT i; // Index number
    CTHANDLE ndx;
    struct ctree_record *ct = CTRecord(self);

    if((i = ctdbGetDefaultIndex(ct->handle)) == -1)
        rb_raise(cCTError, "[%d] ctdbGetDefaultIndex failed.", 
                ctdbGetError(ct->handle));

    if((ndx = ctdbGetIndex(*ct->table_ptr, i)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetIndex failed.", ctdbGetError(ct->handle));

    return rb_ctdb_index_new(cCTIndex, ndx);
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
    return filter ? rb_str_new_cstr(filter) : Qnil;
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
    pCTHANDLE cth = CTRecordH(self);
    
    Check_Type(filter, T_STRING);

    if(ctdbFilterRecord(*cth, RSTRING_PTR(filter)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbFilterRecord failed.", ctdbGetError(*cth));

    return self;
}

/*
 * Indicate if the record is being filtered or not.
 */
static VALUE
rb_ctdb_record_is_filtered(VALUE self)
{
    return ctdbIsFilteredRecord(*CTRecordH(self)) == YES ? Qtrue : Qfalse;
}

/*
 * Find a record using the find mode as the find strategy.  Before using 
 * CT::Record#find:
 *  
 *
 * @param [Fixnum] mode The mode used to look for the record.
 * @raise [CT::Error] ctdbFindRecord failed.
 */
static VALUE
rb_ctdb_record_find(VALUE self, VALUE mode)
{
    pCTHANDLE cth = CTH(self);

    if(ctdbFindRecord(*cth, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbFindRecord failed.", ctdbGetError(*cth));

    return self;
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
 */
static VALUE
rb_ctdb_record_get_field(VALUE self, VALUE field_name)
{
    CTHANDLE field;
    NINT field_nbr;
    CTDBTYPE field_type;
    CTDBRET rc;
    VALUE rb_value = Qnil;
    struct ctree_record *ct = CTRecord(self);

    Check_Type(field_name, T_STRING);

    if((field = ctdbGetFieldByName(*ct->table_ptr, RSTRING_PTR(field_name))) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetFieldByName failed", ctdbGetError(&ct->handle));

    field_nbr  = ctdbGetFieldNbr(field);
    field_type = ctdbGetFieldType(field);
    switch(field_type){
        case CT_BOOL :
            rb_value = rb_funcall(self, rb_intern("get_field_as_bool"), 1, field_name); 
            break;
        case CT_TINYINT :
        case CT_SMALLINT :
        case CT_INTEGER :
        case CT_BIGINT :
            rb_value = rb_funcall(self, rb_intern("get_field_as_signed"), 1, field_name);
            break;
        case CT_UTINYINT :
        case CT_USMALLINT :
        case CT_UINTEGER :
            rb_value = rb_funcall(self, rb_intern("get_field_as_unsigned"), 1, field_name);
            break;
        case CT_CHARS :
        case CT_FPSTRING :
        case CT_F2STRING :
        case CT_F4STRING :
        case CT_PSTRING :
        case CT_VARBINARY :
        case CT_LVB :
        case CT_VARCHAR :
            rb_value = rb_funcall(self, rb_intern("get_field_as_string"), 1, field_name);
            break;
        case CT_DATE :
            rb_value = rb_funcall(self, rb_intern("get_field_as_date"), 1, field_name);
            break;
        case CT_FLOAT :
            rb_value = rb_funcall(self, rb_intern("get_field_as_float"), 1, field_name);
            break;
        case CT_MONEY :
            rb_value = rb_funcall(self, rb_intern("get_field_as_money"), 1, field_name);
            break;
        case CT_TIME :
        case CT_DOUBLE :
        case CT_TIMESTAMP :
        case CT_EFLOAT :
        //case CT_BINARY :
        case CT_NUMBER :
        case CT_CURRENCY :
            rb_raise(rb_eNotImpError, "TODO: get_field field for `%s'", RSTRING_PTR(field_name));
            break;
    }
    return rb_value;
}

/*
 *
 * 
 * @param [Fixnum, String] id The field number or name.
 * @return [Boolean]
 * @raise [CT::Error] ctdbGetFieldAsBool failed.
 */
static VALUE
rb_ctdb_record_get_field_as_bool(VALUE self, VALUE id)
{
    pCTHANDLE cth = CTH(self);
    NINT i; // Field number
    CTBOOL value;

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(ctdbGetFieldAsBool(*cth, i, &value) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsBool failed.", ctdbGetError(*cth));

    return value == YES ? Qtrue : Qfalse;
}

/*
 *
 *
 * @param [Fixnum, String] id The field number or name.
 * @return [CT::Date]
 */
static VALUE
rb_ctdb_record_get_field_as_date(VALUE self, VALUE id)
{
    struct ctree_record* ct;
    NINT i;             // Field number
    CTDBRET rc;         // Generic ctdb return code var
    CTDATE value;       // Hopefully the Date retrieved from the field
    //NINT y, m, d;
    CTUNSIGNED uvalue;  // Container for the field value as unsigned
    CTDATE_TYPE dtype;  // Used to determine the Date format
    CTHANDLE field;     // Field handle
    VRLEN size = 0;     // Size of Date as a String
    TEXT cdt;           // c Date value as a String
    VALUE rbdt;         // The Date as a Ruby object.
    char * format;      // The Date format for strptime
    
    ct = CTRecord(self);

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(ct->handle, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(ct->handle));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }
    
    if(ctdbGetFieldAsUnsigned(ct->handle, i, &uvalue) == CTDBRET_OK){
      
        field = ctdbGetField(*ct->table_ptr, i);
        dtype = ctdbGetFieldDefaultDateType(field);

        switch(dtype) {
            case CTDATE_MDCY :
                format = (char *)"%m/%d/%Y";
                break;
            case CTDATE_DMCY : 
                format = (char *)"%m/%d/%y";
                break;
            case CTDATE_CYMD :
                format = (char *)"%d/%m/%Y";
                break;
            case CTDATE_MDY :
                format = (char *)"%d/%m/%y";
                break;
            case CTDATE_DMY :
                format = (char *)"%Y%m%d";
                break;
            case CTDATE_YMD :
                format = (char *)"%y%m%d";
                break;
            default :
                rb_raise(cCTError, "Unexpected default date format for field `%s'",
                    ctdbGetFieldName(field));
                break;
        }
        
        if(ctdbGetFieldAsDate(ct->handle, i, &value) != CTDBRET_OK)
            rb_raise(cCTError, "[%d] ctdbGetFieldAsDate failed.", 
                ctdbGetError(ct->handle));

        /*
         *if((rc = ctdbDateUnpack(value, &y, &m, &d)) != CTDBRET_OK)
         *    rb_raise(cCTError, "[%d] ctdbDateUnpack failed rc `%d'.", 
         *        ctdbGetError(*cth), rc); 
         *
         *
         *rbdt = rb_funcall(RUBY_CLASS("Date"), rb_intern("new"), 3, 
         *        INT2FIX(y), INT2FIX(m), INT2FIX(d));
         */

        size = (strlen(format) + 3);
        printf("-> size[%d] format[%s] type[%d] [%d]\n", size, format, dtype, value);
        if((rc = ctdbDateToString((CTDATE)value, dtype, &cdt, size)) != CTDBRET_OK)
            rb_raise(cCTError, "[%d] ctdbDateToString failed for `%s'.", rc, 
                    ctdbGetFieldName(field));
        
        rbdt = rb_funcall(RUBY_CLASS("Date"), rb_intern("strptime"), 2, 
                rb_str_new_cstr(&cdt), rb_str_new_cstr(format));
        
    } else {
        rbdt = INT2FIX(0);
    }
    
    return rbdt;
}

/*
 * Retrieve field as Float value.
 *
 * @param [Fixnum, String] id The field number or name.
 * @return [Float]
 * @raise [CT::Error] ctdbGetFieldAsFloat failed.
 */
static VALUE
rb_ctdb_record_get_field_as_float(VALUE self, VALUE id)
{
    NINT i; // Field number
    CTFLOAT value;
    pCTHANDLE cth = CTH(self);

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }


    if(ctdbGetFieldAsFloat(*cth, i, &value) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsFloat failed.", ctdbGetError(*cth));

    return rb_float_new(value);
}

/*
 * Retrieve the field as a signed value.
 *
 * @param [Fixnum, String] id The field number or name.
 * @return [Fixnum]
 * @raise [CT::Error] ctdbGetFieldAsSigned failed.
 */
static VALUE
rb_ctdb_record_get_field_as_signed(VALUE self, VALUE id)
{
    pCTHANDLE cth = CTH(self);
    NINT i; // Field number
    CTSIGNED value;

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(ctdb_record_is_field_null(cth, i) == YES) return Qnil;

    if(ctdbGetFieldAsSigned(*cth, i, &value) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsSigned failed.", ctdbGetError(*cth));

    return INT2FIX(value);
}

/*
 * Retrieve the field as a string value.
 *
 * @param [Fixnum, String] id The field number or name.
 * @return [String]
 * @raise [CT::Error] ctdbGetFieldAsString failed.
 */
static VALUE
rb_ctdb_record_get_field_as_string(VALUE self, VALUE id)
{
    struct ctree_record *ct = CTRecord(self);
    NINT i; // Field number
    VRLEN len;

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(ct->handle, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(ct->handle));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    // Retrieve the actual field size. The actual size of variable-length fields, 
    // such as CT_VARCHAR and CT_VARBINARY, may vary from the defined size.
    len = ctdbGetFieldDataLength(ct->handle, i);

    TEXT value[len+1]; // Field value
    if(ctdbGetFieldAsString(ct->handle, i, value, 
                                            (VRLEN)sizeof(value)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsString failed.", 
                ctdbGetError(ct->handle));

    return rb_str_new_cstr(value);
}

/*
 *
 *
 */
static VALUE
rb_ctdb_record_get_field_as_time(VALUE self, VALUE id)
{
    struct ctree_record *ct = CTRecord(self);
    NINT i; // Field number
    CTTIME value;
    NINT h, m, s;
    CTDBRET rc;

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(ct->handle, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(ct->handle));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(ctdbGetFieldAsTime(ct->handle, i, &value) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsTime failed.", 
                ctdbGetError(ct->handle));

    rc = ctdbTimeUnpack(value, &h, &m, &s);
    if(rc != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbTimeUnpack failed.", rc);

    // VALUE rbtm = rb_funcall(RUBY_CLASS("Time"), rb_intern("new"), 3, 
    //         INT2FIX(h), INT2FIX(m), INT2FIX(s));
    // 
    // return rbtm;

    // TODO: rb_ctdb_record_get_field_as_time
    return self;
}

/*
 *
 * @param [Fixnum, String] id The field number or name.
 * @return [Fixnum]
 * @raise [CT::Error] ctdbGetFieldAsUnsigned failed.
 */
static VALUE
rb_ctdb_record_get_field_as_unsigned(VALUE self, VALUE id)
{
    NINT i;
    CTUNSIGNED value;
    pCTHANDLE cth = CTH(self);

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(ctdb_record_is_field_null(cth, i) == YES) return Qnil;

    if(ctdbGetFieldAsUnsigned(*cth, i, &value) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbGetFieldAsUnsigned failed.", 
                ctdbGetError(*cth));

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
    CTDBRET rc;
    pCTHANDLE cth = CTRecordH(self);

    rc = ctdbNextRecord(*cth);
    if(rc != CTDBRET_OK && rc != INOT_ERR)
        rb_raise(cCTError, "[%d] ctdbNextRecord failed.", ctdbGetError(*cth));

    return rc == INOT_ERR ? Qnil : self;
}

/*
 * Get the previous record on a table.
 *
 * @return [CT::Record, nil] The previous record or nil if the record does not exist.
 */
static VALUE
rb_ctdb_record_prev(VALUE self)
{
    CTDBRET rc;
    pCTHANDLE cth = CTRecordH(self);
    
    rc = ctdbPrevRecord(*cth);
    if(rc != CTDBRET_OK && rc != INOT_ERR)
        rb_raise(cCTError, "[%d] ctdbPrevRecord failed.", ctdbGetError(*cth));

    return rc == INOT_ERR ? Qnil : self;
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
    pCTHANDLE cth = CTH(self);
    CTDBRET rc;

    switch(rb_type(identifier)){
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

    return self;
}

/*
 *
 *
 *
 */
static VALUE
rb_ctdb_record_set_field(VALUE self, VALUE field_name, VALUE value)
{
    struct ctree_record *ct = CTRecord(self);
    CTHANDLE field;
    NINT field_nbr;
    CTDBTYPE field_type;
    
    Check_Type(field_name, T_STRING);

    if((field = ctdbGetFieldByName(*ct->table_ptr, RSTRING_PTR(field_name))) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetFieldByName failed", ctdbGetError(&ct->handle));

    field_nbr  = ctdbGetFieldNbr(field);
    field_type = ctdbGetFieldType(field);
    switch(field_type){
        case CT_BOOL :
            rb_funcall(self, rb_intern("set_field_as_bool"), 2, field_name, value); 
            break;
        case CT_TINYINT :
        case CT_SMALLINT :
        case CT_INTEGER :
        case CT_BIGINT :
            rb_funcall(self, rb_intern("set_field_as_signed"), 2, field_name, value);
            break;
        case CT_UTINYINT :
        case CT_USMALLINT :
        case CT_UINTEGER :
            rb_funcall(self, rb_intern("set_field_as_unsigned"), 2, field_name, value);
            break;
        case CT_CHARS :
        case CT_FPSTRING :
        case CT_F2STRING :
        case CT_F4STRING :
        case CT_PSTRING :
        case CT_VARBINARY :
        case CT_LVB :
        case CT_VARCHAR :
            rb_funcall(self, rb_intern("set_field_as_string"), 2, field_name, value);
            break;
        case CT_DATE :
            rb_funcall(self, rb_intern("set_field_as_date"), 2, field_name, value);
            break;
        case CT_MONEY :
        case CT_TIME :
        case CT_FLOAT :
        case CT_DOUBLE :
        case CT_TIMESTAMP :
        case CT_EFLOAT :
        // NOTE: I kept getting duplicate entry for this case CT_BINARY :
        case CT_NUMBER :
        case CT_CURRENCY :
            rb_raise(rb_eNotImpError, "TODO: set_field for `%s'", RSTRING_PTR(field_name));
            break;
    }
    return self;
}
// static VALUE
// rb_ctdb_record_set_field_as_binary(VALUE self, VALUE num, VALUE value){}

// static VALUE
// rb_ctdb_record_set_field_as_blob(VALUE self, VALUE num, VALUE value){}

/*
 *
 *
 * @param [Fixnum, String] id Field number or name.
 * @param [Boolean] value
 * @raise [CT::Error] ctdbSetFieldAsBool failed.
 */
static VALUE
rb_ctdb_record_set_field_as_bool(VALUE self, VALUE id, VALUE value)
{
    NINT i;
    pCTHANDLE cth = CTRecordH(self);
    CTBOOL cval;

    if(rb_type(value) != T_TRUE && rb_type(value) != T_FALSE)
        rb_raise(rb_eArgError, "Unexpected value type `%s' for CT_BOOL", 
                 rb_obj_classname(value));
    
    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    cval = (rb_type(value) == T_TRUE ? YES : NO);

    if(ctdbSetFieldAsBool(*cth, i, cval) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsBool falied.", ctdbGetError(*cth));

    return self;
}

// static VALUE
// rb_ctdb_record_set_field_as_currency(VALUE self, VALUE num, VALUE value){}

/*
 *
 *
 * @param [Fixnum, String] id Field number or name.
 * @param [Date] value
 */
static VALUE
rb_ctdb_record_set_field_as_date(VALUE self, VALUE id, VALUE value)
{
    NINT i;
    pCTHANDLE cth = CTRecordH(self);
    CTDATE ctdate;
    CTDBRET rc;

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(value == Qnil){
      if(ctdbSetFieldAsUnsigned(*cth, i, 0) != CTDBRET_OK)
            rb_raise(cCTError, "[%d] ctdbSetFieldAsUnsigned failed.",
                ctdbGetError(*cth));
    } else {
        rc = ctdbDatePack(&ctdate, 
            FIX2INT(rb_funcall(value, rb_intern("year"), 0)), 
            FIX2INT(rb_funcall(value, rb_intern("month"), 0)), 
            FIX2INT(rb_funcall(value, rb_intern("day"), 0)));

        if(rc != CTDBRET_OK)
            rb_raise(cCTError, "[%d] ctdbDatePack failed.", rc);
        
        if(ctdbSetFieldAsDate(*cth, i, ctdate) != CTDBRET_OK)
            rb_raise(cCTError, "[%d] ctdbSetFieldAsDate failed.", 
              ctdbGetError(*cth));
    }

    return self;
}

// static VALUE
// rb_ctdb_record_set_field_as_datetime(VALUE self, VALUE num, VALUE value){}

static VALUE
rb_ctdb_record_set_field_as_float(VALUE self, VALUE id, VALUE value)
{
    NINT i;
    pCTHANDLE cth = CTRecordH(self);

    Check_Type(value, T_FLOAT);
    
    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(ctdbSetFieldAsFloat(*cth, i, RFLOAT_VALUE(value)) != CTDBRET_OK)
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
 * @param [Fixnum, String] id Field number or name.
 * @param [Integer] value
 * @raise [CT::Error] ctdbSetFieldAsSigned failed.
 */ 
static VALUE
rb_ctdb_record_set_field_as_signed(VALUE self, VALUE id, VALUE value)
{
    NINT i;
    pCTHANDLE cth = CTRecordH(self);

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(ctdbSetFieldAsSigned(*cth, i, FIX2INT(value)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsSigned failed.", ctdbGetError(*cth));

    return self;
}

/*
 *
 *
 * @param [Fixnum, String] id Field number or name.
 * @param [String] value
 * @raise [CT::Error] ctdbSetFieldAsString failed.
 */
static VALUE
rb_ctdb_record_set_field_as_string(VALUE self, VALUE id, VALUE value)
{
    NINT i;
    struct ctree_record *ct = CTRecord(self);
    TEXT fname;
    CTHANDLE f;
    VRLEN len;
    TEXT padc, delimc;

    Check_Type(value, T_STRING);
    
    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(ct->handle, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(ct->handle));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(!(f = ctdbGetField(*ct->table_ptr, i)))
        rb_raise(cCTError, "[%d] ctdbGetField failed.", ctdbGetError(ct->handle));

    if(NIL_P(value) && ctdbGetFieldNullFlag(f) == NO)
        rb_raise(cCTError, "Field `%s' cannot be NULL.", ctdbGetFieldName(f));

    if(ctdbIsVariableField(ct->handle, i) == NO){
        // Pad string to the fixed length.
        len = ctdbGetFieldLength(f);

        // TODO: Implement dynamic field padding with ctdbGetPadChar(*ct->table_ptr, &padc, &delimc);

        while(RSTRING_LEN(value) <= len-1)
            rb_str_cat2(value, " ");
    }

    if(ctdbSetFieldAsString(ct->handle, i, 
                      (RTEST(value) ? RSTRING_PTR(value) : NULL )) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsString failed.", 
            ctdbGetError(ct->handle));

    return self;
}

/*
 *
 *
 * @param [Fixnum, String] id Field number or name.
 * @param [Time] value
 */
static VALUE
rb_ctdb_record_set_field_as_time(VALUE self, VALUE id, VALUE value)
{
    NINT i;
    struct ctree_record *ct = CTRecord(self);
    TEXT fname;
    CTHANDLE f;
    CTDATE cttime;
    CTDBRET rc;

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(ct->handle, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(ct->handle));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }


    rc = ctdbTimePack(&cttime,
            FIX2INT(rb_funcall(value, rb_intern("hour"), 0)), 
            FIX2INT(rb_funcall(value, rb_intern("min"), 0)), 
            FIX2INT(rb_funcall(value, rb_intern("sec"), 0)));

    if(ctdbSetFieldAsTime(ct->handle, i, cttime) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsTime failed.", 
                ctdbGetError(ct->handle));

    return self;
}

/*
 *
 * Set field as CTUNSIGNED type value.
 *
 * @param [Fixnum, String] id Field number or name.
 * @param [Integer] value
 * @raise [CT::Error] ctdbSetFieldAsUnsigned failed.
 */
static VALUE
rb_ctdb_record_set_field_as_unsigned(VALUE self, VALUE id, VALUE value)
{
    NINT i;
    pCTHANDLE cth = CTRecordH(self);

    switch(rb_type(id)){
        case T_STRING :
            if((i = ctdbGetFieldNumberByName(*cth, RSTRING_PTR(id))) == -1)
                rb_raise(cCTError, "[%d] ctdbGetFieldNumberByName failed.", 
                    ctdbGetError(*cth));
            break;
        case T_FIXNUM :
            i = FIX2INT(id);
            break;
        default:
            rb_raise(rb_eArgError, "Unexpected value type `%s'",
                rb_obj_classname(id));
            break;
    }

    if(ctdbSetFieldAsUnsigned(*cth, i, FIX2INT(value)) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbSetFieldAsUnsigned failed.", ctdbGetError(*cth));

    return self;
}

// static VALUE
// rb_ctdb_record_set_field_as_utf16(VALUE self, VALUE num, VALUE value){}

/*
 * Indicates if record set is active or not.
 */
static VALUE
rb_ctdb_record_is_set(VALUE self)
{
    return ctdbIsRecordSetOn(CTRecordH(self)) == YES ? Qtrue : Qfalse;
}

/*
 * Enable a new record set.  The target key is build from the contents of the 
 * record buffer.
 */
static VALUE
rb_ctdb_record_set_on(VALUE self)
{
    NINT i;             // Index number  
    CTHANDLE index;     // Index handle
    CTHANDLE segment;   // Segment handle
    CTSEG_MODE mode;    // Segment mode
    CTHANDLE field;     // Field handle
    VRLEN count;        // Number of Segments that make up the Index 
    VRLEN len = 0;      // Total length of data in indexed fields
    int j;              // Random counter
    struct ctree_record *ctrec = CTRecord(self);

    if((i = ctdbGetDefaultIndex(ctrec->handle)) == -1)
        rb_raise(cCTError, "[%d] ctdbGetDefaultIndex failed.", 
                ctdbGetError(ctrec->handle));

    printf("default index[%s]\n", ctdbGetDefaultIndexName(ctrec->handle));
    if((index = ctdbGetIndex(*ctrec->table_ptr, i)) == NULL)
        rb_raise(cCTError, "[%d] ctdbGetIndex failed.",
                ctdbGetError(ctrec->handle));

    count = ctdbGetIndexSegmentCount(index);
    printf("count[%d] name[%s]\n", count, ctdbGetIndexName(index));
    for(j = 0; j < count; j++){
        if((segment = ctdbGetSegment(index, j)) == NULL)
            rb_raise(cCTError, "[%d] ctdbGetSegment failed.", 
                    ctdbGetError(index));
        
        // TODO: DRY up old style absolute byte offset jazz.
        mode = ctdbGetSegmentMode(segment);
        if(mode == CTSEG_REGSEG || mode == CTSEG_UREGSEG)
            field = ctdbGetSegmentPartialField(segment);
        else
            field = ctdbGetSegmentField(segment);

        /*
         *printf("==> [%d][%s]\n", j, ctdbGetSegmentFieldName(segment)); 
         *if((field = ctdbGetSegmentField(segment)) == NULL)
         *    rb_raise(cCTError, "[%d] ctdbGetSegmentField failed.",
         *            ctdbGetError(segment));
         */

        if(ctdbGetFieldDataLength(ctrec->handle, ctdbGetFieldNbr(field)) > 0)
            len += ctdbGetFieldLength(field);
    }


    if(ctdbRecordSetOn(ctrec->handle, len) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbRecordSetOn failed.", 
                ctdbGetError(ctrec->handle));

    return self;
}

/*
 * Disable and free an existing record set.
 */
static VALUE
rb_ctdb_record_set_off(VALUE self)
{
    pCTHANDLE cth = CTRecordH(self);

    if(ctdbRecordSetOff(*cth) != CTDBRET_OK)
        rb_raise(cCTError, "[%d] ctdbRecordSetOff failed.", ctdbGetError(*cth));

    return self;
}
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

void 
Init_ctdb_sdk(void)
{
    mCT = rb_define_module("CT");
    // c-treeDB Session Types
    rb_define_const(mCT, "SESSION_CTDB",  INT2FIX(CTSESSION_CTDB));
    rb_define_const(mCT, "SESSION_CTREE", INT2FIX(CTSESSION_CTREE));
    // c-treeDB Find Modes
    rb_define_const(mCT, "FIND_EQ", INT2FIX(CTFIND_EQ));
    rb_define_const(mCT, "FIND_LT", INT2FIX(CTFIND_LT));
    rb_define_const(mCT, "FIND_LE", INT2FIX(CTFIND_LE));
    rb_define_const(mCT, "FIND_GT", INT2FIX(CTFIND_GT));
    rb_define_const(mCT, "FIND_GE", INT2FIX(CTFIND_GE));
    // c-treeDB Lock Modes
    rb_define_const(mCT, "LOCK_FREE",       INT2FIX(CTLOCK_FREE));
    rb_define_const(mCT, "LOCK_READ",       INT2FIX(CTLOCK_READ));
    rb_define_const(mCT, "LOCK_READ_BLOCK", INT2FIX(CTLOCK_READ_BLOCK));
    rb_define_const(mCT, "LOCK_WRITE",      INT2FIX(CTLOCK_WRITE));
    rb_define_const(mCT, "LOCK_WRITE_LOCK", INT2FIX(CTLOCK_WRITE_BLOCK));
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
    rb_define_const(mCT, "INDEX_LEADING", INT2FIX(CTINDEX_LEADING));
    rb_define_const(mCT, "INDEX_PADDING", INT2FIX(CTINDEX_PADDING));
    rb_define_const(mCT, "INDEX_LEADPAD", INT2FIX(CTINDEX_LEADPAD));
    rb_define_const(mCT, "INDEX_ERROR",   INT2FIX(CTINDEX_ERROR));
    // c-treeDB Segment modes
    rb_define_const(mCT, "SEG_SCHSEG",     INT2FIX(CTSEG_SCHSEG));
    rb_define_const(mCT, "SET_USCHSEG",    INT2FIX(CTSEG_USCHSEG));
    rb_define_const(mCT, "SEG_VSCHSEG",    INT2FIX(CTSEG_VSCHSEG));
    rb_define_const(mCT, "SEG_UVSCHSEG",   INT2FIX(CTSEG_UVSCHSEG));
    rb_define_const(mCT, "SEG_SCHSRL",     INT2FIX(CTSEG_SCHSRL));
    rb_define_const(mCT, "SEG_DESCENDING", INT2FIX(CTSEG_DESCENDING));
    rb_define_const(mCT, "SEG_ALTSEG",     INT2FIX(CTSEG_ALTSEG));
    rb_define_const(mCT, "SEG_ENDSEG",     INT2FIX(CTSEG_ENDSEG));
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
    rb_define_const(mCT, "DB_ALTER_NORMAL",   INT2FIX(CTDB_ALTER_NORMAL));
    rb_define_const(mCT, "DB_ALTER_INDEX",    INT2FIX(CTDB_ALTER_INDEX));
    rb_define_const(mCT, "DB_ALTER_FULL",     INT2FIX(CTDB_ALTER_FULL));
    rb_define_const(mCT, "DB_ALTER_PURGEDUP", INT2FIX(CTDB_ALTER_PURGEDUP));
    // c-treeDB Table status
    rb_define_const(mCT, "DB_REBUILD_NONE",     INT2FIX(CTDB_REBUILD_NONE));
    rb_define_const(mCT, "DB_REBUILD_DODA",     INT2FIX(CTDB_REBUILD_DODA));
    rb_define_const(mCT, "DB_REBUILD_RESOURCE", INT2FIX(CTDB_REBUILD_RESOURCE));
    rb_define_const(mCT, "DB_REBUILD_INDEX",    INT2FIX(CTDB_REBUILD_INDEX));
    rb_define_const(mCT, "DB_REBUILD_ALL",      INT2FIX(CTDB_REBUILD_ALL));
    rb_define_const(mCT, "DB_REBUILD_FULL",     INT2FIX(CTDB_REBUILD_FULL));

    // CT::Error
    cCTError = rb_define_class_under(mCT, "Error", rb_eStandardError);

    // CT::Session
    cCTSession = rb_define_class_under(mCT, "Session", rb_cObject);
    rb_define_singleton_method(cCTSession, "new", rb_ctdb_session_new, 1);
    rb_define_method(cCTSession, "active?", rb_ctdb_session_is_active, 0);
    rb_define_method(cCTSession, "default_date_type", rb_ctdb_get_defualt_date_type, 0);
    rb_define_method(cCTSession, "default_date_type=", rb_ctdb_set_defualt_date_type, 1);
    // rb_define_method(cCTSession, "delete_table", rb_ctdb_session_delete_table, 1);
    rb_define_method(cCTSession, "find_active_table", rb_ctdb_session_find_active_table, 1);
    rb_define_method(cCTSession, "lock", rb_ctdb_session_lock, 1);
    rb_define_method(cCTSession, "lock!", rb_ctdb_session_lock_bang, 1);
    rb_define_method(cCTSession, "locked?", rb_ctdb_session_is_locked, 0);
    rb_define_method(cCTSession, "logon", rb_ctdb_session_logon, 3);
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
    rb_define_method(cCTTable, "get_index", rb_ctdb_table_get_index, 1);
    //rb_define_method(cCTTable, "indecies", rb_ctdb_table_get_indecies, 0); 
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
    rb_define_method(cCTTable, "name", rb_ctdb_table_get_name, 0);
    rb_define_method(cCTTable, "open", rb_ctdb_table_open, 2);
    rb_define_method(cCTTable, "active?", rb_ctdb_table_is_active, 0);
    rb_define_alias(cCTTable, "open?", "active?");
    
    rb_define_method(cCTTable, "pad_char", rb_ctdb_table_get_pad_char, 0);
    rb_define_method(cCTTable, "pad_char=", rb_ctdb_table_set_pad_char, 1);
    rb_define_method(cCTTable, "path", rb_ctdb_table_get_path, 0);
    rb_define_method(cCTTable, "path=", rb_ctdb_table_set_path, 1);
    rb_define_method(cCTTable, "rebuild", rb_ctdb_table_rebuild, 1);
    rb_define_method(cCTTable, "status", rb_ctdb_table_get_status, 0);

    // CT::Field
    cCTField = rb_define_class_under(mCT, "Field", rb_cObject);
    // rb_define_singleton_method(cCTField, "new", rb_ctdb_field_new, -1);
    rb_define_method(cCTField, "allow_nil?", rb_ctdb_field_get_null_flag, 0);
    rb_define_method(cCTField, "default", rb_ctdb_field_get_default, 0);
    rb_define_method(cCTField, "default=", rb_ctdb_field_set_default, 1);
    rb_define_method(cCTField, "inspect", rb_ctdb_field_inspect, 0);
    rb_define_method(cCTField, "length", rb_ctdb_field_get_length, 0);
    rb_define_method(cCTField, "length=", rb_ctdb_field_set_length, 1);
    rb_define_method(cCTField, "name", rb_ctdb_field_get_name, 0);
    rb_define_method(cCTField, "name=", rb_ctdb_field_set_name, 1);
    rb_define_method(cCTField, "number", rb_ctdb_field_get_number, 0);
    rb_define_method(cCTField, "precision", rb_ctdb_field_get_precision, 0);
    rb_define_method(cCTField, "precision=", rb_ctdb_field_set_precision, 1);
    rb_define_method(cCTField, "properties", rb_ctdb_field_get_properties, 0);
    rb_define_method(cCTField, "scale", rb_ctdb_field_get_scale, 0);
    rb_define_method(cCTField, "scale=", rb_ctdb_field_set_scale, 1);
    rb_define_method(cCTField, "type", rb_ctdb_field_get_type, 0);
    rb_define_method(cCTField, "type=", rb_ctdb_field_set_type, 1);
    // rb_define_method(cCTField, "variable_length?", rb_ctdb_field_is_variable, 0);

    // CT::Index
    cCTIndex = rb_define_class_under(mCT, "Index", rb_cObject);
    // rb_define_singleton_method(cCTTable, "new", rb_ctdb_index_init, 4);
    rb_define_method(cCTIndex, "add_segment", rb_ctdb_index_add_segment, 2);
    rb_define_method(cCTIndex, "allow_dups?", rb_ctdb_index_get_allow_dups, 1);
    rb_define_method(cCTIndex, "allow_dups=", rb_ctdb_index_set_allow_dups, 1);
    rb_define_method(cCTIndex, "key_length", rb_ctdb_index_get_key_length, 0);
    rb_define_method(cCTIndex, "key_type",   rb_ctdb_index_get_key_type, 0);
    rb_define_method(cCTIndex, "name", rb_ctdb_index_get_name, 0);
    rb_define_method(cCTIndex, "segments", rb_ctdb_index_get_segments, 0);
    // rb_define_method(cCTIndex, "number", rb_ctdb_index_get_number, 0);

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
    rb_define_method(cCTSegment, "absolute_byte_offset?", rb_ctdb_segment_absolute_byte_offset, 0);

    // CT::Record
    cCTRecord = rb_define_class_under(mCT, "Record", rb_cObject);
    rb_define_singleton_method(cCTRecord, "new", rb_ctdb_record_new, 1);
    rb_define_method(cCTRecord, "initialize", rb_ctdb_record_init, 1);
    rb_define_method(cCTRecord, "clear", rb_ctdb_record_clear, 0);
    rb_define_method(cCTRecord, "count", rb_ctdb_record_get_count, 0);
    rb_define_method(cCTRecord, "default_index", rb_ctdb_record_get_default_index, 0);
    rb_define_method(cCTRecord, "default_index=", rb_ctdb_record_set_default_index, 1);
    rb_define_method(cCTRecord, "filter", rb_ctdb_record_get_filter, 0);
    rb_define_method(cCTRecord, "filter=", rb_ctdb_record_set_filter, 1);
    rb_define_method(cCTRecord, "filtered?", rb_ctdb_record_is_filtered, 0);
    rb_define_method(cCTRecord, "find", rb_ctdb_record_find, 1);
    rb_define_method(cCTRecord, "first", rb_ctdb_record_first, 0);
    rb_define_method(cCTRecord, "first!", rb_ctdb_record_first_bang, 0);
    rb_define_method(cCTRecord, "get_field", rb_ctdb_record_get_field, 1);
    rb_define_alias(cCTRecord,  "[]", "get_field");
    rb_define_method(cCTRecord, "get_field_as_bool", rb_ctdb_record_get_field_as_bool, 1);
    rb_define_method(cCTRecord, "get_field_as_date", rb_ctdb_record_get_field_as_date, 1);
    rb_define_method(cCTRecord, "get_field_as_float", rb_ctdb_record_get_field_as_float, 1);
    rb_define_alias(cCTRecord, "get_field_as_money", "get_field_as_float");
    rb_define_method(cCTRecord, "get_field_as_signed", rb_ctdb_record_get_field_as_signed, 1);
    rb_define_method(cCTRecord, "get_field_as_string", rb_ctdb_record_get_field_as_string, 1);
    rb_define_method(cCTRecord, "get_field_as_time", rb_ctdb_record_get_field_as_time, 1);
    rb_define_method(cCTRecord, "get_field_as_unsigned", rb_ctdb_record_get_field_as_unsigned, 1);
    rb_define_method(cCTRecord, "last", rb_ctdb_record_last, 0);
    rb_define_method(cCTRecord, "last!", rb_ctdb_record_last_bang, 0);
    rb_define_method(cCTRecord, "lock", rb_ctdb_record_lock, 1);
    rb_define_method(cCTRecord, "lock!", rb_ctdb_record_lock_bang, 1);
    rb_define_method(cCTRecord, "next", rb_ctdb_record_next, 0);
    rb_define_method(cCTRecord, "prev", rb_ctdb_record_prev, 0);
    rb_define_method(cCTRecord, "set_field", rb_ctdb_record_set_field, 2);
    rb_define_alias(cCTRecord,  "[]=", "set_field");
    rb_define_method(cCTRecord, "set_field_as_bool", rb_ctdb_record_set_field_as_bool, 2);
    rb_define_method(cCTRecord, "set_field_as_date", rb_ctdb_record_set_field_as_date, 2);
    rb_define_method(cCTRecord, "set_field_as_float", rb_ctdb_record_set_field_as_float, 2);
    rb_define_method(cCTRecord, "set_field_as_signed", rb_ctdb_record_set_field_as_signed, 2);
    rb_define_method(cCTRecord, "set_field_as_string", rb_ctdb_record_set_field_as_string, 2);
    rb_define_method(cCTRecord, "set_field_as_time", rb_ctdb_record_set_field_as_time, 2);
    rb_define_method(cCTRecord, "set_field_as_unsigned", rb_ctdb_record_set_field_as_unsigned, 2);
    rb_define_method(cCTRecord, "set?", rb_ctdb_record_is_set, 0);
    rb_define_method(cCTRecord, "set_on", rb_ctdb_record_set_on, 0);
    rb_define_method(cCTRecord, "set_off", rb_ctdb_record_set_off, 0);
    rb_define_method(cCTRecord, "write!", rb_ctdb_record_write_bang, 0);
}
