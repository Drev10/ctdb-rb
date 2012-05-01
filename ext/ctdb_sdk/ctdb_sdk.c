#include "ruby.h"
#include "ctdbsdk.h" /* c-tree headers */

#define GetCtree(obj)       (Check_Type(obj, T_DATA), (struct ctree*)DATA_PTR(obj))
#define GetCtreeRecord(obj) (Check_Type(obj, T_DATA), (struct ctree_record*)DATA_PTR(obj))

VALUE mCtree;
VALUE cCtreeError;    // CTDB::Error
VALUE cCtreeSession;  // CTDB::Session
VALUE cCtreeTable;    // CTDB::Table
VALUE cCtreeIndex;    // CTDB::Index
VALUE cCtreeRecord;   // CTDB::Record
VALUE cCtreeField;    // CTDB::Field

struct ctree {
    CTHANDLE handle;
};

struct ctree_record {
    CTHANDLE handle;
    CTHANDLE table;
};

// ==================
// = CTDB::Session =
// ==================
static void 
free_ctree_session(struct ctree* ct)
{
    ctdbFreeSession(ct->handle);
    xfree(ct);
}

// CTDB::Session.new
static VALUE
ctree_session_init(VALUE klass)
{
    struct ctree* ct;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree_session, ct);
    // Allocate a new session for logon only. No session or database dictionary
    // files will be used. No database functions can be used with this session mode.
    if((ct->handle = ctdbAllocSession(CTSESSION_CTREE)) == NULL)
        rb_raise(cCtreeError, "ctdbAllocSession failed.");

    rb_obj_call_init(obj, 0, NULL); // CTDB::Session.initialize
    return obj;
}

// CTDB::Session#logon(host, username, password)
static VALUE 
ctree_session_logon(int argc, VALUE *argv, VALUE obj)
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

// CTDB::Session#logout
static VALUE 
ctree_session_logout(VALUE obj)
{
    CTHANDLE session = GetCtree(obj)->handle;
    ctdbLogout(session);
    return obj;
}

// CTDB::Session#active?
static VALUE 
ctree_session_is_active(VALUE obj)
{
    CTHANDLE session = GetCtree(obj)->handle;
    return ctdbIsActiveSession(session) ? Qtrue : Qfalse;
}

// CTDB::Session#lock(mode)
static VALUE
ctree_session_lock(VALUE obj, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    CTHANDLE ct = GetCtree(obj)->handle;
    return ctdbLock(ct, FIX2INT(mode)) == CTDBRET_OK ? Qtrue : Qfalse;
}

// CTDB::Session#lock!(mode)
static VALUE
ctree_session_lock_bang(VALUE obj, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbLock(ct, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLock failed.", ctdbGetError(ct));

    return Qtrue;
}

// CTDB::Session#locked?
static VALUE
ctree_session_is_locked(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    return ctdbIsLockActive(ct) == YES ? Qtrue : Qfalse;
}

// CTDB::Session#unlock
static VALUE
ctree_session_unlock(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    return ctdbUnlock(ct) == CTDBRET_OK ? Qtrue : Qfalse;
}

// CTDB::Session#unlock!
static VALUE
ctree_session_unlock_bang(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbUnlock(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbUnlock failed.", ctdbGetError(ct));

    return Qtrue;
}

// CTDB::Session#path_prefix
static VALUE
ctree_session_get_path_prefix(VALUE obj)
{
    pTEXT prefix = ctdbGetPathPrefix(GetCtree(obj)->handle);
    return prefix ? rb_str_new2(prefix) : Qnil;
}

// CTDB::Session#path_prefix=(prefix)
static VALUE
ctree_session_set_path_prefix(VALUE obj, VALUE prefix)
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

    return Qtrue;
}

// ================
// = CTDB::Table =
// ================
static void
free_ctree_table(struct ctree* ct)
{
    ctdbFreeTable(ct->handle);
    xfree(ct);
}

// CTDB::Table.new
static VALUE
ctree_table_init(VALUE klass, VALUE session)
{
    Check_Type(session, T_DATA);

    struct ctree* ct;
    CTHANDLE cth = GetCtree(session)->handle;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree_table, ct);

    if((ct->handle = ctdbAllocTable(cth)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbAllocTable failed", ctdbGetError(cth));

    rb_obj_call_init(obj, 0, NULL); // CTDB::Table.initialize
    return obj;
}

// CTDB::Table#path=(value)
static VALUE
ctree_table_set_path(VALUE obj, VALUE path)
{
    Check_Type(path, T_STRING);

    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbSetTablePath(ct, RSTRING_PTR(path)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbSetTablePath failed.", ctdbGetError(ct));

    return Qnil;
}

// CTDB::Table#path
static VALUE
ctree_table_get_path(VALUE obj)
{
    VALUE s = rb_str_new2(ctdbGetTablePath(GetCtree(obj)->handle));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

// CTDB::Table#name
static VALUE
ctree_table_get_name(VALUE obj)
{
    VALUE s = rb_str_new2(ctdbGetTableName(GetCtree(obj)->handle));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

// CTDB::Table#index(identifier)
// static VALUE
// ctree_table_get_index(VALUE obj, VALUE identifier)
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

// CTDB::Table#open(name)
static VALUE
ctree_table_open(VALUE obj, VALUE name)
{
    Check_Type(name, T_STRING);

    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbOpenTable(ct, RSTRING_PTR(name), CTOPEN_NORMAL) !=  CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbOpenTable failed.", ctdbGetError(ct));

    return obj;
}

// CTDB::Table#field_names
static VALUE
ctree_table_get_field_names(VALUE obj)
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

// CTDB::Table#close
static VALUE
ctree_table_close(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;

    if(ctdbCloseTable(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbCloseTable failed.", ctdbGetError(ct));

    return Qtrue;
}

// ================
// = CTDB::Index =
// ================
static void
free_ctree_index(struct ctree* ct)
{
    xfree(ct);
}

// // CTDB::Index.new
// static VALUE
// ctree_index_init(VALUE klass, VALUE name, VALUE type, VALUE unique, VALUE is_null)
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
//     rb_obj_call_init(obj, 0, NULL); // CTDB::Index.initialize
//     return obj;
// }

// CTDB::Index#type
static VALUE
ctree_index_get_key_type(VALUE obj)
{
    return ctdbGetIndexKeyType(GetCtree(obj)->handle);
}

// CTDB::Index#name
static VALUE
ctree_index_get_name(VALUE obj)
{
    pTEXT name = ctdbGetIndexName(GetCtree(obj)->handle);
    return name ? rb_str_new2(name) : Qnil;
}

// CTDB::Index#length
static VALUE
ctree_index_get_key_length(VALUE obj)
{
    VRLEN len;
    len = ctdbGetIndexKeyLength(GetCtree(obj)->handle);
    return len == -1 ? Qnil : INT2FIX(len);
}

// CTDB::Index#unique?
static VALUE
ctree_index_is_unique(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    return ctdbGetIndexDuplicateFlag(ct) == YES ? Qfalse : Qtrue;
}

// =================
// = CTDB::Record =
// =================
static void
free_ctree_record(struct ctree_record* ctrec)
{
    ctdbFreeRecord(ctrec->handle);
    xfree(ctrec);
}

// CTDB::Record.new
static VALUE
ctree_record_init(VALUE klass, VALUE table)
{
    Check_Type(table, T_DATA);

    struct ctree_record* ctrec;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree_record, 0, free_ctree_record, ctrec);
    ctrec->table = GetCtree(table)->handle;
    if((ctrec->handle = ctdbAllocRecord(ctrec->table)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbAllocRecord failed.", 
                 ctdbGetError(ctrec->table));

    rb_obj_call_init(obj, 0, NULL); // CTDB::Record.initialize
    return obj;
}

// CTDB::Record#default_index
static VALUE
ctree_record_get_default_index(VALUE obj)
{
    CTHANDLE ndx;
    pTEXT name;

    ndx  = ctdbGetDefaultIndexName(GetCtreeRecord(obj)->table);
    name = ctdbGetIndexName(ndx);
    return name ? rb_str_new2(name) : Qnil;
}

// CTDB::Record#default_index=(identifier)
static VALUE
ctree_record_set_default_index(VALUE obj, VALUE identifier)
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

// CTDB::Record#first
static VALUE
ctree_record_first(VALUE obj)
{
    return ctdbFirstRecord(GetCtreeRecord(obj)->handle) == CTDBRET_OK ? obj : Qnil;
}

// CTDB::Record#first!
static VALUE
ctree_record_first_bang(VALUE obj)
{
    CTHANDLE ct = GetCtreeRecord(obj)->handle;

    if(ctdbFirstRecord(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbFirstRecord failed.", ctdbGetError(ct));

    return obj;
}

// CTDB::Record#last
static VALUE
ctree_record_last(VALUE obj)
{
    return ctdbLastRecord(GetCtreeRecord(obj)->handle) == CTDBRET_OK ? obj : Qnil;
}

// CTDB::Record#last!
static VALUE
ctree_record_last_bang(VALUE obj)
{
    CTHANDLE ct = GetCtreeRecord(obj)->handle;
    if(ctdbLastRecord(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLastRecord failed.", ctdbGetError(ct));

    return obj;
}

// CTDB::Record#next
static VALUE
ctree_record_next(VALUE obj)
{
    return ctdbNextRecord(GetCtreeRecord(obj)->handle) == CTDBRET_OK ? obj : Qnil;
}

// CTDB::Record#prev
static VALUE
ctree_record_prev(VALUE obj)
{
    return ctdbPrevRecord(GetCtreeRecord(obj)->handle) == CTDBRET_OK ? obj : Qnil;
}

// CTDB::Record#field(field)
static VALUE
ctree_record_get_field(VALUE obj, VALUE name)
{
    struct ctree_record *ctrec = GetCtreeRecord(obj);
    CTHANDLE field;
    CTDBRET rc;
    int n, len;  // field number and data length
    void* cval;  // generic pointer to store field data
    cval = ALLOC(void*);

    VALUE rbval; // field data converted to ruby data-type

    if((n = ctdbGetFieldNumber(ctrec->table, RSTRING_PTR(name))) == -1)
        rb_raise(cCtreeError, "[%d] ctdbGetFieldNumber failed.", 
                 ctdbGetError(ctrec->handle));

    if((field = ctdbGetField(ctrec->table, n)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbGetField failed.", 
                 ctdbGetError(ctrec->table));

    switch(ctdbGetFieldType(field)){
        case CT_CHARS :
        case CT_FSTRING :
    //     case CT_FPSTRING :
    //     case CT_F2STRING :
    //     case CT_F4STRING :
    //     case CT_PSTRING :
    //     case CT_2STRING :
    //     case CT_4STRING :
        case CT_VARCHAR :
        case CT_LVC :
        case CT_STRING :
            len = ctdbGetFieldDataLength(ctrec->handle, n);
            rc = ctdbGetFieldAsString(ctrec->handle, n, (CTSTRING)cval, (len+1));
            rbval = rb_str_new2(cval);
            break;
    //     case CT_DATE :
    //         rc = ctdbGetFieldAsDate(ct, num, ()cval);
    //         break;
    //     case CT_TIME : 
    //         rc = ctdbGetFieldAsTime(ct, num, ()cval);
    //         break;
    //     case CT_UTINYINT :
    //     case CT_CHARU :
    //     case CT_USMALLINT :
    //     case CT_INT2U :
    //     case CT_UINTEGER :
    //     case CT_INT4U :
    //         rc = ctdbGetFieldAsUnsigned(ct, num, ()cval);
    //         break;
        case CT_TINYINT :
        case CT_CHAR :
        case CT_SMALLINT :
        case CT_INT2 :
        case CT_INTEGER :
        case CT_INT4 :
            rc = ctdbGetFieldAsSigned(ctrec->handle, n, (pCTSIGNED)&cval);
            rbval = INT2NUM(cval);
            break;
        default :
            rb_raise(cCtreeError, "Unknown data-type for field `%s'", 
                     RSTRING_PTR(name));
            break;
    }
    if(rc != CTDBRET_OK)
        rb_raise(cCtreeError, "FOO BAR!");

    return rbval;
}


// CTDB::Session#set(field, value)
static VALUE
ctree_record_set_field(VALUE obj, VALUE name, VALUE value)
{
    struct ctree_record *ctrec = GetCtreeRecord(obj);
    CTHANDLE field;
    NINT n;
    CTDBRET rc;

    if((field = ctdbGetFieldByName(ctrec->table, RSTRING_PTR(name))) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbGetFieldByName failed", 
                 ctdbGetError(ctrec->handle));

    if((n = ctdbGetFieldNbr(field)) == -1)
        rb_raise(cCtreeError, "[%d] ctdbGetFieldNbr failed.", 
                 ctdbGetError(ctrec->handle));

    switch(ctdbGetFieldType(field)){
        case CT_STRING :
        case CT_FSTRING :
            Check_Type(value, T_STRING);
            rc = ctdbSetFieldAsString(ctrec->handle, n, 
                                      (CTSTRING)RSTRING_PTR(value));
            break;
        case CT_INT4 :
            Check_Type(value, T_FIXNUM);
            rc = ctdbSetFieldAsSigned(ctrec->handle, n, (CTSIGNED)FIX2INT(value));
            break;
        default :
            rb_raise(cCtreeError, "Unknown data-type for field `%s'", 
                     RSTRING_PTR(name));
            break;
    }

    if(rc != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] wtf?", ctdbGetError(ctrec->handle));

    return Qtrue;
}

// CTDB::Record#write
static VALUE
ctree_record_write(VALUE obj)
{
    CTHANDLE ct = GetCtree(obj)->handle;
    if(ctdbWriteRecord(ct))
        rb_raise(cCtreeError, "[%d] ctdbWriteRecord failed.", ctdbGetError(ct));

    return Qtrue;
}

// ================
// = CTDB::Field =
// ================
// static void
// free_ctree_field(struct ctree_field* ct)
// {
// }

void 
Init_ctree_sdk(void)
{
    mCtree = rb_define_module("CTDB");
    // c-treeDB Find Modes
    rb_define_const(mCtree, "FIND_EQ", CTFIND_EQ);
    rb_define_const(mCtree, "FIND_LT", CTFIND_LT);
    rb_define_const(mCtree, "FIND_LE", CTFIND_LE);
    rb_define_const(mCtree, "FIND_GT", CTFIND_GT);
    rb_define_const(mCtree, "FIND_GE", CTFIND_GE);
    // c-treeDB Lock Modes
    rb_define_const(mCtree, "LOCK_FREE", CTLOCK_FREE);
    rb_define_const(mCtree, "LOCK_READ", CTLOCK_READ);
    rb_define_const(mCtree, "LOCK_READ_BLOCK", CTLOCK_READ_BLOCK);
    rb_define_const(mCtree, "LOCK_WRITE", CTLOCK_WRITE);
    rb_define_const(mCtree, "LOCK_WRITE_LOCK", CTLOCK_WRITE_BLOCK);
    // c-treeDB Index Types
    rb_define_const(mCtree, "INDEX_FIXED", CTINDEX_FIXED);
    rb_define_const(mCtree, "INDEX_LEADING", CTINDEX_LEADING);
    rb_define_const(mCtree, "INDEX_PADDING", CTINDEX_PADDING);
    rb_define_const(mCtree, "INDEX_LEADPAD", CTINDEX_LEADPAD);
    rb_define_const(mCtree, "INDEX_ERROR", CTINDEX_ERROR);

    // CTDB::Error
    cCtreeError = rb_define_class_under(mCtree, "Error", rb_eStandardError);

    // CTDB::Session
    cCtreeSession = rb_define_class_under(mCtree, "Session", rb_cObject);
    rb_define_singleton_method(cCtreeSession, "new", ctree_session_init, 0);
    rb_define_method(cCtreeSession, "logon", ctree_session_logon, -1);
    rb_define_method(cCtreeSession, "logout", ctree_session_logout, 0);
    rb_define_method(cCtreeSession, "active?", ctree_session_is_active, 0);
    rb_define_method(cCtreeSession, "lock", ctree_session_lock, 1);
    rb_define_method(cCtreeSession, "lock!", ctree_session_lock_bang, 1);
    rb_define_method(cCtreeSession, "locked?", ctree_session_is_locked, 0);
    rb_define_method(cCtreeSession, "unlock", ctree_session_unlock, 0);
    rb_define_method(cCtreeSession, "unlock!", ctree_session_unlock_bang, 0);
    rb_define_method(cCtreeSession, "path_prefix", ctree_session_get_path_prefix, 0);
    rb_define_method(cCtreeSession, "path_prefix=", ctree_session_set_path_prefix, 1);

    // CTDB::Table
    cCtreeTable = rb_define_class_under(mCtree, "Table", rb_cObject);
    rb_define_singleton_method(cCtreeTable, "new", ctree_table_init, 1);
    rb_define_method(cCtreeTable, "path=", ctree_table_set_path, 1);
    rb_define_method(cCtreeTable, "path", ctree_table_get_path, 0);
    rb_define_method(cCtreeTable, "name", ctree_table_get_name, 0);
    rb_define_method(cCtreeTable, "open", ctree_table_open, 1);
    rb_define_method(cCtreeTable, "close", ctree_table_close, 0);
    // rb_define_method(cCtreeTable, "index", ctree_table_get_index, 1);
    // rb_define_method(cCtreeTable, "indecies", ctree_table_get_indecies, 0);
    rb_define_method(cCtreeTable, "field_names", ctree_table_get_field_names, 0);

    // CTDB::Index
    cCtreeIndex = rb_define_class_under(mCtree, "Index", rb_cObject);
    // rb_define_singleton_method(cCtreeTable, "new", ctree_index_init, 4);
    rb_define_method(cCtreeIndex, "name", ctree_index_get_name, 0);
    // rb_define_method(cCtreeIndex, "number", ctree_index_get_number, 0);
    rb_define_method(cCtreeIndex, "key_type",   ctree_index_get_key_type, 0);
    rb_define_method(cCtreeIndex, "key_length", ctree_index_get_key_length, 0);
    rb_define_method(cCtreeIndex, "unique?", ctree_index_is_unique, 0);

    // CTDB::Record
    cCtreeRecord = rb_define_class_under(mCtree, "Record", rb_cObject);
    rb_define_singleton_method(cCtreeRecord, "new", ctree_record_init, 1);
    rb_define_method(cCtreeRecord, "default_index", ctree_record_get_default_index, 0);
    rb_define_method(cCtreeRecord, "default_index=", ctree_record_set_default_index, 1);
    rb_define_method(cCtreeRecord, "first", ctree_record_first, 0);
    rb_define_method(cCtreeRecord, "first!", ctree_record_first_bang, 0);
    rb_define_method(cCtreeRecord, "last", ctree_record_last, 0);
    rb_define_method(cCtreeRecord, "last!", ctree_record_last_bang, 0);
    rb_define_method(cCtreeRecord, "next", ctree_record_next, 0);
    rb_define_method(cCtreeRecord, "prev", ctree_record_prev, 0);
    rb_define_method(cCtreeRecord, "get_field", ctree_record_get_field, 1);
    rb_define_method(cCtreeRecord, "set_field", ctree_record_set_field, 2);

    // CTDB::Field
    // cCtreeField = rb_define_class_under(mCtree, "Field", rb_cObject);
    // rb_define_singleton_method(cCtreeRecord, "new", ctree_field_init, -1);
}