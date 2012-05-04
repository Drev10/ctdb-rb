#include "ruby.h"
#include "ctdbsdk.h" /* c-tree headers */

#define GetCtree(obj)       (Check_Type(obj, T_DATA), (struct ctree*)DATA_PTR(obj))
#define GetCtreeRecord(obj) (Check_Type(obj, T_DATA), (struct ctree_record*)DATA_PTR(obj))

VALUE mCtree;
VALUE cCtreeError;    // CT::Error
VALUE cCtreeSession;  // CT::Session
VALUE cCtreeTable;    // CT::Table
VALUE cCtreeIndex;    // CT::Index
VALUE cCtreeRecord;   // CT::Record
VALUE cCtreeField;    // CT::Field

struct ctree {
    CTHANDLE handle;
};

struct ctree_record {
    CTHANDLE handle;
    CTHANDLE table;
};

// ==================
// = CT::Session =
// ==================
static void 
free_rb_ctdb_session(struct ctree* ct)
{
    ctdbFreeSession(ct->handle);
    xfree(ct);
}

static VALUE
rb_ctdb_session_init(VALUE klass)
{
    struct ctree* ct;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree_session, ct);
    // Allocate a new session for logon only. No session or database dictionary
    // files will be used. No database functions can be used with this session mode.
    if((ct->handle = ctdbAllocSession(CTSESSION_CTREE)) == NULL)
        rb_raise(cCtreeError, "ctdbAllocSession failed.");

    rb_obj_call_init(obj, 0, NULL); // CT::Session.initialize
    return obj;
}

/*
 * Retrieve the active state of a table.  A table is active if it is open.
 */
static VALUE 
rb_ctdb_session_is_active(VALUE obj)
{
    CTHANDLE session = GetCtree(obj)->handle;
    return ctdbIsActiveSession(session) ? Qtrue : Qfalse;
}

/* 
 * Perform a session-wide lock.
 *
 * @param [Fixnum] mode A c-treeDB lock mode.
 */
static VALUE
rb_ctdb_session_lock(VALUE obj, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    CTHANDLE ct = GetCtree(obj)->handle;
    return ctdbLock(ct, FIX2INT(mode)) == CTDBRET_OK ? Qtrue : Qfalse;
}

/*
 * @see #lock
 * @raise [CT::Error] ctdbLock failed.
 */
static VALUE
ctree_session_lock_bang(VALUE obj, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbLock(ct, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLock failed.", ctdbGetError(ct));

    return Qtrue;
}

/* 
 * Check to see if a lock is active
 */
static VALUE
rb_ctdb_session_is_locked(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    return ctdbIsLockActive(ct) == YES ? Qtrue : Qfalse;
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
rb_ctdb_session_logon(int argc, VALUE *argv, VALUE obj)
{
    VALUE host, user, pass;
    CTHANDLE session = GetCtree(obj)->handle;

    rb_scan_args(argc, argv, "30", &host, &user, &pass);
    char *h = RSTRING_PTR(host);
    char *u = RSTRING_PTR(user);
    char *p = RSTRING_PTR(pass);

    if(ctdbLogon(session, h, u, p) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLogon failed.", ctdbGetError(session));

    return obj;
}

/*
 * Logout from a c-tree Server session or from a c-treeACE instance.
 */
static VALUE 
rb_ctdb_session_logout(VALUE obj)
{
    CTHANDLE session = GetCtree(obj)->handle;
    ctdbLogout(session);
    return obj;
}

/*
 * Returns the client-side path prefix.
 * 
 * @return [String, nil] The path name or nil if no path is set.
 */
static VALUE
rb_ctdb_session_get_path_prefix(VALUE obj)
{
    pTEXT prefix = ctdbGetPathPrefix(GetCtree(obj)->handle);
    return prefix ? rb_str_new2(prefix) : Qnil;
}

/*
 * Set the client-side path prefix.
 *
 * @param [String] prefix Path.
 * @raise [CT::Error] ctdbSetPathPrefix failed.
 */
static VALUE
rb_ctdb_session_set_path_prefix(VALUE obj, VALUE prefix)
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

    CTHANDLE session = GetCtree(obj)->handle;

    if(ctdbSetPathPrefix(session, pp) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbSetPathPrefix failed.", 
            ctdbGetError(session));

    return obj;
}

/*
 * Releases all session-wide locks
 *
 * @return [Boolean]
 */
static VALUE
rb_ctdb_session_unlock(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    return ctdbUnlock(ct) == CTDBRET_OK ? Qtrue : Qfalse;
}

/* 
 * @see #unlock
 * @raise [CT::Error] ctdbUnlock failed.
 */
static VALUE
rb_ctdb_session_unlock_bang(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbUnlock(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbUnlock failed.", ctdbGetError(ct));

    return Qtrue;
}

// ================
// = CT::Table =
// ================
static void
free_rb_ctdb_table(struct ctree* ct)
{
    ctdbFreeTable(ct->handle);
    xfree(ct);
}

/*
 * @param [CT::Session]
 */
static VALUE
rb_ctdb_table_init(VALUE klass, VALUE session)
{
    Check_Type(session, T_DATA);

    struct ctree* ct;
    CTHANDLE cth = GetCtree(session)->handle;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree_table, ct);

    if((ct->handle = ctdbAllocTable(cth)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbAllocTable failed", ctdbGetError(cth));

    rb_obj_call_init(obj, 0, NULL); // CT::Table.initialize
    return obj;
}

/*
 * Rebuild existing table based on field, index and segment changes.
 *
 * @param [Fixnum] mode The alter table action
 * @raise [CT::Error] ctdbAlterTable failed.
 */
static VALUE
rb_ctdb_table_alter(VALUE obj, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbAlterTable(ct, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbAlterTable failed.", ctdbGetError(ct));

    return obj;
}

/*
 * Close the table.
 *
 * @raise [CT::Error] ctdbCloseTable failed.
 */
static VALUE
rb_ctdb_table_close(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbCloseTable(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbCloseTable failed.", ctdbGetError(ct));

    return obj;
}

/*
 * Retrieve the table delimiter character.
 *
 * @return [String] The field delimiter character.
 * @raise [CT::Error] ctdbGetPadChar failed.
 */
static VALUE rb_ctdb_table_get_delim_char(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    pTEXT dchar;

    if(ctdbGetPadChar(ct, NULL, dchar) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbGetPadChar failed.", ctdbGetError(ct));

    return rb_str_new2(dchar);
}

/*
 * Retrieve the number of fields associated with the table. 
 *
 * @return [Fixnum]
 * @raise [CT::Error] ctdbGetTableFieldCount failed.
 */
static VALUE
rb_ctdb_table_get_field_count(VALUE obj)
{
    int i;
    CTHANDLE ct = GetCtree(obj)->handle;

    if((i = ctdbGetTableFieldCount(ct)) == -1)
        rb_raise(cCtreeError, "[%d] ctdbGetTableFieldCount failed.", 
                 ctdbGetError(ct));

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
rb_ctdb_table_get_field_names(VALUE obj)
{
    int i, n;
    CTHANDLE table = GetCtree(obj)->handle;
    CTHANDLE field;
    pTEXT fname;
    VALUE fields;

    if((n = ctdbGetTableFieldCount(table)) == -1)
        rb_raise(cCtreeError, "[%d] ctdbGetTableFieldCount failed.", 
            ctdbGetError(table));

    fields = rb_ary_new2(n);

    for(i = 0; i < n; i++){
        // Get the field handle.
        if((field = ctdbGetField(table, i)) == NULL)
            rb_raise(cCtreeError, "[%d] ctdbGetField failed.", 
                ctdbGetError(table));
        // Use the field handle to retrieve the field name.
        if((fname = ctdbGetFieldName(field)) == NULL)
            rb_raise(cCtreeError, "[%d] ctdbGetFieldName failed.", 
                ctdbGetError(table));
        // Store the field name
        rb_ary_store(fields, i, rb_str_new2(fname));
    }
    return fields;
}

/*
 * Retrieve the table name.
 *
 * @return [String]
 */
static VALUE
rb_ctdb_table_get_name(VALUE obj)
{
    VALUE s = rb_str_new2(ctdbGetTableName(GetCtree(obj)->handle));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

/*
 * Open the table.
 *
 * @param [String] name Name of the table to open
 * @raise [CT::Error] ctdbOpenTable failed.
 */
static VALUE
rb_ctdb_table_open(VALUE obj, VALUE name)
{
    Check_Type(name, T_STRING);

    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbOpenTable(ct, RSTRING_PTR(name), CTOPEN_NORMAL) !=  CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbOpenTable failed.", ctdbGetError(ct));

    return obj;
}

/*
 * Retrieve the table pad character
 *
 * @return [String] The character.
 * @raise [CT::Error] ctdbGetPadChar failed
 */
static VALUE
rb_ctdb_table_get_pad_char(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    pTEXT pchar;

    if(ctdbGetPadChar(ct, pchar, NULL) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbGetPadChar failed.", ctdbGetError(ct));

    return rb_str_new2(pchar);
}

/*
 * Retrieve the table path.
 *
 * @return [String, nil] The path or nil if one is not set.
 */
static VALUE
rb_ctdb_table_get_path(VALUE obj)
{
    VALUE s = rb_str_new2(ctdbGetTablePath(GetCtree(obj)->handle));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

/*
 * Set a new table path
 *
 * @param [String] path
 * @raise [CT::Error] ctdbSetTablePath failed.
 */
static VALUE
rb_ctdb_table_set_path(VALUE obj, VALUE path)
{
    Check_Type(path, T_STRING);

    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbSetTablePath(ct, RSTRING_PTR(path)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbSetTablePath failed.", ctdbGetError(ct));

    return Qnil;
}

// static VALUE
// rb_ctdb_table_get_index(VALUE obj, VALUE identifier)
// {
//     struct ctree *index;
//     CTHANDLE ct = GetCtree(obj)->handle;
//     VALUE iobj;
//     obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree_table, ct);
// 
//     switch(TYPE(identifier)){
//         case T_STRING :
//         case T_SYMBOL :
//             ndx = ctdbGetIndexByName(ct, RSTRING_PTR(RSTRING(identifier)));
//               break;
//           case T_FIXNUM :
//               ndx = ctdbGetIndex(ct, FIX2INT(identifier));
//               break;
//           default :
//               rb_raise(rb_eArgError, "Unexpected value type `%s'", 
//                        rb_obj_classname(identifier));
//               break;
//       }
// }

// static VALUE
// rb_ctdb_table_add_field(VALUE obj, VALUE name, VALUE type, VALUE size)
// {
//     Check_Type(name, T_STRING);
//     Check_Type(size, T_FIXNUM);
// 
//     CTHANDLE ct = GetCtree(obj)->handle;
// 
//     // ctdbAddField(ct, RSTRING_PTR(name),   type, );
// }

// static VALUE
// rb_ctdb_table_get_field(VALUE obj)
// {
// }

/*
 * Retrieves the table status. The table status indicates which rebuild action 
 * will be taken by an alter table operation.
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_table_get_status(VALUE obj)
{
    return INT2FIX(ctdbGetTableStatus(GetCtree(obj)->handle));
}

// ================
// = CT::Index =
// ================
static void
free_rb_ctdb_index(struct ctree* ct)
{
    xfree(ct);
}

// static VALUE
// rb_ctdb_index_init(VALUE klass, VALUE name, VALUE type, VALUE unique, VALUE is_null)
// {
//     Check_Type(type, T_DATA);
// 
//     struct ctree* ct;
//     VALUE obj;
// 
//     obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree_index, ct);
// 
//     if((ct->handle = ctdbAllocTable(cth)) == NULL)
//         rb_raise(cCtreeError, "[%d] ctdbAllocTable failed", ctdbGetError(cth));
// 
//     rb_obj_call_init(obj, 0, NULL); // CT::Index.initialize
//     return obj;
// }

/*
 * Retrieve the key type for this index.
 *
 * @return [Fixnum]
 */
static VALUE
rb_ctdb_index_get_key_type(VALUE obj)
{
    return INT2FIX(ctdbGetIndexKeyType(GetCtree(obj)->handle));
}

/*
 * Retrieve the index name
 *
 * @return [String, nil]
 */
static VALUE
rb_ctdb_index_get_name(VALUE obj)
{
    pTEXT name = ctdbGetIndexName(GetCtree(obj)->handle);
    return name ? rb_str_new2(name) : Qnil;
}

/*
 * Retrieve the key length for this index.
 *
 * @return [Fixnum, nil]
 */
static VALUE
rb_ctdb_index_get_key_length(VALUE obj)
{
    VRLEN len;
    len = ctdbGetIndexKeyLength(GetCtree(obj)->handle);
    return len == -1 ? Qnil : INT2FIX(len);
}

/*
 * Check if the duplicate flag is set to false.
 */
static VALUE
rb_ctdb_index_is_unique(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    return ctdbGetIndexDuplicateFlag(ct) == YES ? Qfalse : Qtrue;
}

// =================
// = CT::Record =
// =================
static void
free_rb_ctdb_record(struct ctree_record* ctrec)
{
    ctdbFreeRecord(ctrec->handle);
    xfree(ctrec);
}

static VALUE
rb_ctdb_record_init(VALUE klass, VALUE table)
{
    Check_Type(table, T_DATA);

    struct ctree_record* ctrec;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree_record, 0, free_ctree_record, ctrec);
    ctrec->table = GetCtree(table)->handle;
    if((ctrec->handle = ctdbAllocRecord(ctrec->table)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbAllocRecord failed.", 
                 ctdbGetError(ctrec->table));

    rb_obj_call_init(obj, 0, NULL); // CT::Record.initialize
    return obj;
}

/*
 * Retrieves the current default index name. When the record handle is initialized 
 * for the first time, the default index is set to zero.
 *
 * @return [String, nil]
 */
static VALUE
rb_ctdb_record_get_default_index(VALUE obj)
{
    CTHANDLE ndx;
    pTEXT name;

    ndx  = ctdbGetDefaultIndexName(GetCtreeRecord(obj)->table);
    name = ctdbGetIndexName(ndx);
    return name ? rb_str_new2(name) : Qnil;
}

/*
 * Set the new default index by name or number.
 *
 * @param [String, Symbol, Fixnum] The index identifier.
 * @raise [CT::Error] ctdbSetDefaultIndexByName or ctdbSetDefaultIndex failed.
 */
static VALUE
rb_ctdb_record_set_default_index(VALUE obj, VALUE identifier)
{
    CTHANDLE ct = GetCtreeRecord(obj)->table;
    CTDBRET rc;

    switch(TYPE(identifier)){
        case T_STRING :
        case T_SYMBOL :
            rc = ctdbSetDefaultIndexByName(ct, RSTRING_PTR(RSTRING(identifier)));
            break;
        case T_FIXNUM :
            rc = ctdbSetDefaultIndex(ct, FIX2INT(identifier));
            break;
        default :
            rb_raise(rb_eArgError, "Unexpected value type `%s'", 
                     rb_obj_classname(identifier));
            break;
    }

    if(rc != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbSetDefaultIndex failed.", ctdbGetError(ct));

    return Qtrue;
}

/*
 * Get the first record on a table
 */
static VALUE
rb_ctdb_record_first(VALUE obj)
{
    return ctdbFirstRecord(GetCtreeRecord(obj)->handle) == CTDBRET_OK ? obj : Qnil;
}

/*
 * @see #first
 * @raise [CT::Error] ctdbFirstRecord failed.
 */
static VALUE
rb_ctdb_record_first_bang(VALUE obj)
{
    CTHANDLE ct = GetCtreeRecord(obj)->handle;

    if(ctdbFirstRecord(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbFirstRecord failed.", ctdbGetError(ct));

    return obj;
}

/*
 * Retrieve field value.
 *
 * @param [String] name Field name
 * @raise [CT::Error]
 */
static VALUE
rb_ctdb_record_get_field(VALUE obj, VALUE name)
{
    struct ctree_record *ctrec = GetCtreeRecord(obj);
    CTHANDLE field;
    CTDBRET rc;
    CTBOOL null_flag;
    int n, l;    // field number and data length
    void* cval;  // generic pointer to store field data
    cval = ALLOC(void*);

    VALUE rbval; // field data converted to ruby data-type

    if((n = ctdbGetFieldNumber(ctrec->table, RSTRING_PTR(name))) == -1)
        rb_raise(cCtreeError, "[%d] ctdbGetFieldNumber failed.", 
                 ctdbGetError(ctrec->handle));

    if((field = ctdbGetField(ctrec->table, n)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbGetField failed.", 
                 ctdbGetError(ctrec->table));

    null_flag = ctdbGetFieldNullFlag(field);

    switch(ctdbGetFieldType(field)){
        case CT_BOOL :
            rc = ctdbGetFieldAsBool(ctrec->handle, n, (pCTBOOL)&cval);
            rbval = (((CTBOOL)cval == YES) ? Qtrue : Qfalse);
            break;
        case CT_CHARS :
    //     case CT_FPSTRING :
    //     case CT_F2STRING :
    //     case CT_F4STRING :
            l = ctdbGetFieldDataLength(ctrec->handle, n);
            rc = ctdbGetFieldAsString(ctrec->handle, n, (CTSTRING)cval, (l+1));
            rbval = rb_str_new2(cval);
            break;
    //     case CT_DATE :
    //         rc = ctdbGetFieldAsDate(ct, num, ()cval);
    //         break;
    //     case CT_TIME : 
    //         rc = ctdbGetFieldAsTime(ct, num, ()cval);
    //         break;
        case CT_UTINYINT :
        case CT_USMALLINT :
        case CT_UINTEGER :
            rc = ctdbGetFieldAsUnsigned(ctrec->handle, n, (pCTUNSIGNED)&cval);
            rbval = UINT2NUM(cval);
            break;
        case CT_TINYINT :
        case CT_SMALLINT :
        case CT_INTEGER :
            rc = ctdbGetFieldAsSigned(ctrec->handle, n, (pCTSIGNED)&cval);
            rbval = INT2NUM(cval);
            break;
        case CT_PSTRING :
        case CT_VARBINARY :
        case CT_LVB :
        case CT_VARCHAR :
            l = ctdbGetFieldDataLength(ctrec->handle, n);
            rc = ctdbGetFieldAsString(ctrec->handle, n, (CTSTRING)cval, (l+1));
            rbval = rb_str_new2(cval);
            break;
        default :
            rb_raise(cCtreeError, "Uhandled data-type for field `%s'", 
                     RSTRING_PTR(name));
            break;
    }
    if(rc != CTDBRET_OK)
        rb_raise(cCtreeError, "FOO BAR!");

    return rbval;
}

/*
 * Get the last record on a table.
 *
 * @return [CT::Record, nil]
 */
static VALUE
rb_ctdb_record_last(VALUE obj)
{
    return ctdbLastRecord(GetCtreeRecord(obj)->handle) == CTDBRET_OK ? obj : Qnil;
}

/*
 * @see #last
 * @raise [CT::Error] ctdbLastRecord failed.
 */
static VALUE
rb_ctdb_record_last_bang(VALUE obj)
{
    CTHANDLE ct = GetCtreeRecord(obj)->handle;
    if(ctdbLastRecord(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLastRecord failed.", ctdbGetError(ct));

    return obj;
}

/*
 * Lock the current record.
 *
 * @param [Fixnum] mode The record lock mode.
 */
static VALUE
rb_ctdb_record_lock(VALUE obj, VALUE mode)
{
    CTHANDLE ct = GetCtreeRecord(obj)->handle;
    return ((ctdbLockRecord(ct, FIX2INT(mode)) == CTDBRET_OK) ? Qtrue : Qfalse);
}

/*
 * @see #lock
 * @raise [CT::Error] ctdbLockRecord failed.
 */
static VALUE
rb_ctdb_record_lock_bang(VALUE obj, VALUE mode)
{
    CTHANDLE ct = GetCtreeRecord(obj)->handle;

    if(ctdbLockRecord(ct, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLockRecord failed.", ctdbGetError(ct));

    return Qtrue;
}

/*
 * Get the next record on a table.
 *
 * @return [CT::Record, nil]
 */
static VALUE
rb_ctdb_record_next(VALUE obj)
{
    return ctdbNextRecord(GetCtreeRecord(obj)->handle) == CTDBRET_OK ? obj : Qnil;
}

/*
 * Get the previous record on a table.
 *
 * @return [CT::Record, nil] The previous record or nil if the record does not exist.
 */
static VALUE
rb_ctdb_record_prev(VALUE obj)
{
    return ctdbPrevRecord(GetCtreeRecord(obj)->handle) == CTDBRET_OK ? obj : Qnil;
}

/*
 * Set the field value
 *
 * @param [String] name The field name
 * @param [Object] value The field value55
 */
static VALUE
rb_ctdb_record_set_field(VALUE obj, VALUE name, VALUE value)
{
    struct ctree_record *ctrec = GetCtreeRecord(obj);
    CTHANDLE field;
    NINT n;     // field number
    CTDBRET rc; // return code for all ctdbSetField calls

    if((field = ctdbGetFieldByName(ctrec->table, RSTRING_PTR(name))) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbGetFieldByName failed", 
                 ctdbGetError(ctrec->handle));

    if((n = ctdbGetFieldNbr(field)) == -1)
        rb_raise(cCtreeError, "[%d] ctdbGetFieldNbr failed.", 
                 ctdbGetError(ctrec->handle));

    switch(ctdbGetFieldType(field)){
        case CT_BOOL :
            if(TYPE(value) != T_TRUE && TYPE(value) != T_FALSE)
                rb_raise(rb_eArgError, "Unexpected value type `%s' for CT_BOOL", 
                         rb_obj_classname(value));

            rc = ctdbSetFieldAsBool(ctrec->handle, n, 
                                   (CTBOOL)((TYPE(value) == T_TRUE) ? YES : NO));
            break;
        case CT_TINYINT :
        case CT_SMALLINT :
        case CT_INTEGER :
            Check_Type(value, T_FIXNUM);
            rc = ctdbSetFieldAsSigned(ctrec->handle, n, (CTSIGNED)FIX2INT(value));
            break;
        case CT_UTINYINT :
        case CT_USMALLINT :
        case CT_UINTEGER :
            Check_Type(value, T_FIXNUM);
            rc = ctdbSetFieldAsUnsigned(ctrec->handle, n, 
                                        (CTUNSIGNED)FIX2INT(value));
            break;
        // case CT_MONEY :
        //     rc = ctdbSetFieldAsMoney(ctrec->handle, n )
        //     break;
        // case CT_DATE :
        //     rc = ctdbSetFieldAsDate(ctrec->handle, n);
        //     break;
        // case CT_TIME :
        //     rc = ctdbSetFieldAsTime(ctrec->handle, n);
        //     break;
        case CT_FLOAT :
        case CT_DOUBLE :
        // case CT_EFLOAT :
            Check_Type(value, T_FLOAT);
            rc = ctdbSetFieldAsFloat(ctrec->handle, n, 
                                     (CTFLOAT)RFLOAT_VALUE(value));
            break;
        // case CT_TIMESTAMP :
        //      break;

        TEXT pchar, dchar;
        VRLEN l;
        case CT_CHARS :
        case CT_FPSTRING :
        case CT_F2STRING :
        case CT_F4STRING :
            Check_Type(value, T_STRING);
            // Get the field length
            // if((l = ctdbGetFieldLength(field)) == -1)
            //     rb_raise(cCtreeError, "[%d] ctdbGetFieldLength failed.", 
            //              ctdbGetError(field));
            // printf("[%d]\n", __LINE__);
            // // Get the character we will use to pad the string to length
            // if((ctdbGetPadChar(ctrec->table, &pchar, &dchar)) != CTDBRET_OK)
            //     rb_raise(cCtreeError, "[%d] ctdbGetPadChar failed.", 
            //              ctdbGetError(ctrec->table));
            // printf("[%d][%c][%c]\n", __LINE__, pchar, dchar);
            // // Make moves
            // REALLOC_N(cval, char, (l+1));
            // for(;strlen(cval) < (l+1))
            //     strcat(cval, pchar);
            // 
            // rc = ctdbSetFieldAsString(ctrec->handle, n, 
            //                           (CTSTRING));
            break;
        case CT_PSTRING :
        case CT_VARBINARY :
        case CT_LVB :
        case CT_VARCHAR :
            Check_Type(value, T_STRING);
            rc = ctdbSetFieldAsString(ctrec->handle, n, 
                                      (CTSTRING)RSTRING_PTR(value));
            break;
        default :
            rb_raise(cCtreeError, "Unhandled data-type for field `%s'", 
                     RSTRING_PTR(name));
            break;
    }

    if(rc != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] wtf?", ctdbGetError(ctrec->handle));

    return Qtrue;
}

/*
 * Create or update an existing record.
 *
 * @raise [CT::Error] ctdbWriteRecord failed.
 */
static VALUE
rb_ctdb_record_write_bang(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    if(ctdbWriteRecord(ct))
        rb_raise(cCtreeError, "[%d] ctdbWriteRecord failed.", ctdbGetError(ct));

    return Qtrue;
}

// ================
// = CT::Field =
// ================
// static void
// free_rb_ctdb_field(struct ctree_field* ct)
// {
// }

void 
Init_ctdb_sdk(void)
{
    mCtree = rb_define_module("CT");
    // c-treeDB Find Modes
    rb_define_const(mCtree, "FIND_EQ", CTFIND_EQ);
    rb_define_const(mCtree, "FIND_LT", CTFIND_LT);
    rb_define_const(mCtree, "FIND_LE", CTFIND_LE);
    rb_define_const(mCtree, "FIND_GT", CTFIND_GT);
    rb_define_const(mCtree, "FIND_GE", CTFIND_GE);
    // c-treeDB Lock Modes
    rb_define_const(mCtree, "LOCK_FREE",       CTLOCK_FREE);
    rb_define_const(mCtree, "LOCK_READ",       CTLOCK_READ);
    rb_define_const(mCtree, "LOCK_READ_BLOCK", CTLOCK_READ_BLOCK);
    rb_define_const(mCtree, "LOCK_WRITE",      CTLOCK_WRITE);
    rb_define_const(mCtree, "LOCK_WRITE_LOCK", CTLOCK_WRITE_BLOCK);
    // c-treeDB Index Types
    rb_define_const(mCtree, "INDEX_FIXED",   CTINDEX_FIXED);
    rb_define_const(mCtree, "INDEX_LEADING", CTINDEX_LEADING);
    rb_define_const(mCtree, "INDEX_PADDING", CTINDEX_PADDING);
    rb_define_const(mCtree, "INDEX_LEADPAD", CTINDEX_LEADPAD);
    rb_define_const(mCtree, "INDEX_ERROR",   CTINDEX_ERROR);
    // c-ctreeDB Field Types
    rb_define_const(mCtree, "BOOL",      CT_BOOL);
    rb_define_const(mCtree, "TINYINT",   CT_TINYINT);
    rb_define_const(mCtree, "UTINYINT",  CT_UTINYINT);
    rb_define_const(mCtree, "SMALLINT",  CT_SMALLINT);
    rb_define_const(mCtree, "USMALLINT", CT_USMALLINT);
    rb_define_const(mCtree, "INTEGER",   CT_INTEGER);
    rb_define_const(mCtree, "UINTEGER",  CT_UINTEGER);
    rb_define_const(mCtree, "MONEY",     CT_MONEY);
    rb_define_const(mCtree, "DATE",      CT_DATE);
    rb_define_const(mCtree, "TIME",      CT_TIME);
    rb_define_const(mCtree, "FLOAT",     CT_FLOAT);
    rb_define_const(mCtree, "DOUBLE",    CT_DOUBLE);
    rb_define_const(mCtree, "TIMESTAMP", CT_TIMESTAMP);
    rb_define_const(mCtree, "EFLOAT",    CT_EFLOAT);
    rb_define_const(mCtree, "BINARY",    CT_BINARY);
    rb_define_const(mCtree, "CHARS",     CT_CHARS);
    rb_define_const(mCtree, "FPSTRING",  CT_FPSTRING);
    rb_define_const(mCtree, "F2STRING",  CT_F2STRING);
    rb_define_const(mCtree, "F4STRING",  CT_F4STRING);
    rb_define_const(mCtree, "BIGINT",    CT_BIGINT);
    rb_define_const(mCtree, "NUMBER",    CT_NUMBER);
    rb_define_const(mCtree, "CURRENCY",  CT_CURRENCY);
    rb_define_const(mCtree, "PSTRING",   CT_PSTRING);
    rb_define_const(mCtree, "VARBINARY", CT_VARBINARY);
    rb_define_const(mCtree, "LVB",       CT_LVB);
    rb_define_const(mCtree, "VARCHAR",   CT_VARCHAR);
    rb_define_const(mCtree, "LVC",       CT_LVC);
    // c-ctreeDB Table alter modes
    rb_define_const(mCtree, "DB_ALTER_NORMAL",   CTDB_ALTER_NORMAL);
    rb_define_const(mCtree, "DB_ALTER_INDEX",    CTDB_ALTER_INDEX);
    rb_define_const(mCtree, "DB_ALTER_FULL",     CTDB_ALTER_FULL);
    rb_define_const(mCtree, "DB_ALTER_PURGEDUP", CTDB_ALTER_PURGEDUP);
    // c-treeDB Table status
    rb_define_const(mCtree, "DB_REBUILD_NONE",     CTDB_REBUILD_NONE);
    rb_define_const(mCtree, "DB_REBUILD_DODA",     CTDB_REBUILD_DODA);
    rb_define_const(mCtree, "DB_REBUILD_RESOURCE", CTDB_REBUILD_RESOURCE);
    rb_define_const(mCtree, "DB_REBUILD_INDEX",    CTDB_REBUILD_INDEX);
    rb_define_const(mCtree, "DB_REBUILD_ALL",      CTDB_REBUILD_ALL);
    rb_define_const(mCtree, "DB_REBUILD_FULL",     CTDB_REBUILD_FULL);
    // CT::Error
    cCtreeError = rb_define_class_under(mCtree, "Error", rb_eStandardError);

    // CT::Session
    cCtreeSession = rb_define_class_under(mCtree, "Session", rb_cObject);
    rb_define_singleton_method(cCtreeSession, "new", rb_ctdb_session_init, 0);
    rb_define_method(cCtreeSession, "active?", rb_ctdb_session_is_active, 0);
    rb_define_method(cCtreeSession, "lock", rb_ctdb_session_lock, 1);
    rb_define_method(cCtreeSession, "lock!", rb_ctdb_session_lock_bang, 1);
    rb_define_method(cCtreeSession, "locked?", rb_ctdb_session_is_locked, 0);
    rb_define_method(cCtreeSession, "logon", rb_ctdb_session_logon, -1);
    rb_define_method(cCtreeSession, "logout", rb_ctdb_session_logout, 0);
    rb_define_method(cCtreeSession, "path_prefix", rb_ctdb_session_get_path_prefix, 0);
    rb_define_method(cCtreeSession, "path_prefix=", rb_ctdb_session_set_path_prefix, 1);
    rb_define_method(cCtreeSession, "unlock", rb_ctdb_session_unlock, 0);
    rb_define_method(cCtreeSession, "unlock!", rb_ctdb_session_unlock_bang, 0);

    // CT::Table
    cCtreeTable = rb_define_class_under(mCtree, "Table", rb_cObject);
    rb_define_singleton_method(cCtreeTable, "new", rb_ctdb_table_init, 1);
    rb_define_method(cCtreeTable, "alter", rb_ctdb_table_alter, 1);
    rb_define_method(cCtreeTable, "close", rb_ctdb_table_close, 0);
    rb_define_method(cCtreeTable, "delim_char", rb_ctdb_table_get_delim_char, 0);
    rb_define_method(cCtreeTable, "field_count", rb_ctdb_table_get_field_count, 0);
    rb_define_method(cCtreeTable, "field_names", rb_ctdb_table_get_field_names, 0);
    // rb_define_method(cCtreeTable, "indecies", rb_ctdb_table_get_indecies, 0);
    // rb_define_method(cCtreeTable, "index", rb_ctdb_table_get_index, 1);
    rb_define_method(cCtreeTable, "name", rb_ctdb_table_get_name, 0);
    rb_define_method(cCtreeTable, "open", rb_ctdb_table_open, 1);
    rb_define_method(cCtreeTable, "pad_char", rb_ctdb_table_get_pad_char, 0);
    rb_define_method(cCtreeTable, "path", rb_ctdb_table_get_path, 0);
    rb_define_method(cCtreeTable, "path=", rb_ctdb_table_set_path, 1);
    rb_define_method(cCtreeTable, "status", rb_ctdb_table_get_status, 0);

    // CT::Index
    cCtreeIndex = rb_define_class_under(mCtree, "Index", rb_cObject);
    // rb_define_singleton_method(cCtreeTable, "new", rb_ctdb_index_init, 4);
    rb_define_method(cCtreeIndex, "key_length", rb_ctdb_index_get_key_length, 0);
    rb_define_method(cCtreeIndex, "key_type",   rb_ctdb_index_get_key_type, 0);
    rb_define_method(cCtreeIndex, "name", rb_ctdb_index_get_name, 0);
    // rb_define_method(cCtreeIndex, "number", rb_ctdb_index_get_number, 0);
    rb_define_method(cCtreeIndex, "unique?", rb_ctdb_index_is_unique, 0);

    // CT::Record
    cCtreeRecord = rb_define_class_under(mCtree, "Record", rb_cObject);
    rb_define_singleton_method(cCtreeRecord, "new", rb_ctdb_record_init, 1);
    rb_define_method(cCtreeRecord, "default_index", rb_ctdb_record_get_default_index, 0);
    rb_define_method(cCtreeRecord, "default_index=", rb_ctdb_record_set_default_index, 1);
    rb_define_method(cCtreeRecord, "first", rb_ctdb_record_first, 0);
    rb_define_method(cCtreeRecord, "first!", rb_ctdb_record_first_bang, 0);
    rb_define_method(cCtreeRecord, "get_field", rb_ctdb_record_get_field, 1);
    rb_define_method(cCtreeRecord, "last", rb_ctdb_record_last, 0);
    rb_define_method(cCtreeRecord, "last!", rb_ctdb_record_last_bang, 0);
    rb_define_method(cCtreeRecord, "lock", rb_ctdb_record_lock, 1);
    rb_define_method(cCtreeRecord, "lock!", rb_ctdb_record_lock_bang, 1);
    rb_define_method(cCtreeRecord, "next", rb_ctdb_record_next, 0);
    rb_define_method(cCtreeRecord, "prev", rb_ctdb_record_prev, 0);
    rb_define_method(cCtreeRecord, "set_field", rb_ctdb_record_set_field, 2);
    rb_define_method(cCtreeRecord, "write!", rb_ctdb_record_write_bang, 0);

    // CT::Field
    // cCtreeField = rb_define_class_under(mCtree, "Field", rb_cObject);
    // rb_define_singleton_method(cCtreeRecord, "new", rb_ctdb_field_init, -1);
}