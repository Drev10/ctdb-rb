#include "ruby.h"
#include "ctdbsdk.h" /* c-tree headers */

#define GetCtree(obj)       (Check_Type(obj, T_DATA), (struct ctree*)DATA_PTR(obj))
#define GetCtreeRecord(obj) (Check_Type(obj, T_DATA), (struct ctree_record*)DATA_PTR(obj))

VALUE mCtree;
VALUE cCtreeError;    // Ctree::Error
VALUE cCtreeSession;  // Ctree::Session
VALUE cCtreeTable;    // Ctree::Table
VALUE cCtreeRecord;   // Ctree::Record
VALUE cCtreeField;    // Ctree::Field

struct ctree {
    CTHANDLE handle;
};

struct ctree_record {
    CTHANDLE handle;
    CTHANDLE table;
};

/*
 * Ctree::Session
 */
static void 
free_ctree_session(struct ctree* ct)
{
    ctdbFreeSession(ct->handle);
    xfree(ct);
}

// Ctree::Session.new
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

    rb_obj_call_init(obj, 0, NULL); // Ctree::Session.initialize
    return obj;
}

// Ctree::Session#logon(host, username, password)
static VALUE 
ctree_session_logon(int argc, VALUE *argv, VALUE obj)
{
    VALUE host, user, pass;
    CTHANDLE* session = GetCtree(obj)->handle;

    rb_scan_args(argc, argv, "30", &host, &user, &pass);
    char *h = RSTRING_PTR(host);
    char *u = RSTRING_PTR(user);
    char *p = RSTRING_PTR(pass);

    if(ctdbLogon(session, h, u, p) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLogon failed.", ctdbGetError(&session));

    return obj;
}

// Ctree::Session#logout
static VALUE 
ctree_session_logout(VALUE obj)
{
    CTHANDLE* session = GetCtree(obj)->handle;
    ctdbLogout(session);
    return obj;
}

// Ctree::Session#active?
static VALUE 
ctree_session_is_active(VALUE obj)
{
    CTHANDLE* session = GetCtree(obj)->handle;
    return ctdbIsActiveSession(session) ? Qtrue : Qfalse;
}

// Ctree::Session#lock(mode)
static VALUE
ctree_session_lock(VALUE obj, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    CTHANDLE *ct = GetCtree(obj)->handle;
    return ctdbLock(ct, FIX2INT(mode)) == CTDBRET_OK ? Qtrue : Qfalse;
}

// Ctree::Session#lock!(mode)
static VALUE
ctree_session_lock_bang(VALUE obj, VALUE mode)
{
    Check_Type(mode, T_FIXNUM);

    CTHANDLE *ct = GetCtree(obj)->handle;

    if(ctdbLock(ct, FIX2INT(mode)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLock failed.", ctdbGetError(&ct));

    return Qtrue;
}

// Ctree::Session#locked?
static VALUE
ctree_session_is_locked(VALUE obj)
{
    CTHANDLE *ct = GetCtree(obj)->handle;
    return ctdbIsLockActive(ct) == YES ? Qtrue : Qfalse;
}

// Ctree::Session#unlock
static VALUE
ctree_session_unlock(VALUE obj)
{
    CTHANDLE *ct = GetCtree(obj)->handle;
    return ctdbUnlock(ct) == CTDBRET_OK ? Qtrue : Qfalse;
}

// Ctree::Session#unlock!
static VALUE
ctree_session_unlock_bang(VALUE obj)
{
    CTHANDLE *ct = GetCtree(obj)->handle;

    if(ctdbUnlock(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbUnlock failed.", ctdbGetError(&ct));

    return Qtrue;
}

/*
 * Ctree::Table
 */
static void
free_ctree_table(struct ctree* ct)
{
    ctdbFreeTable(ct->handle);
    xfree(ct);
}

// Ctree::Table.new
static VALUE
ctree_table_init(VALUE klass, VALUE session)
{
    Check_Type(session, T_DATA);

    struct ctree* ct;
    CTHANDLE *cth = GetCtree(session)->handle;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree_table, ct);

    if((ct->handle = ctdbAllocTable(cth)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbAllocTable failed", ctdbGetError(&cth));

    rb_obj_call_init(obj, 0, NULL); // Ctree::Table.initialize
    return obj;
}

// Ctree::Table#path=(value)
static VALUE
ctree_table_set_path(VALUE obj, VALUE path)
{
    Check_Type(path, T_STRING);

    CTHANDLE *ct = GetCtree(obj)->handle;

    if(ctdbSetTablePath(ct, RSTRING_PTR(path)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbSetTablePath failed.", ctdbGetError(&ct));

    return Qnil;
}

// Ctree::Table#path
static VALUE
ctree_table_get_path(VALUE obj)
{
    VALUE s = rb_str_new2(ctdbGetTablePath(GetCtree(obj)->handle));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

// Ctree::Table#name
static VALUE
ctree_table_get_name(VALUE obj)
{
    VALUE s = rb_str_new2(ctdbGetTableName(GetCtree(obj)->handle));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

// Ctree::Table#open(name)
static VALUE
ctree_table_open(VALUE obj, VALUE name)
{
    Check_Type(name, T_STRING);

    CTHANDLE *ct = GetCtree(obj)->handle;

    if(ctdbOpenTable(ct, RSTRING_PTR(name), CTOPEN_NORMAL) !=  CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbOpenTable failed.", ctdbGetError(&ct));

    return obj;
}

// Ctree::Table#close
static VALUE
ctree_table_close(VALUE obj)
{
    CTHANDLE *ct = GetCtree(obj)->handle;

    if(ctdbCloseTable(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbCloseTable failed.", ctdbGetError(&ct));

    return Qtrue;
}

/*
 * Ctree::Record
 */
static void 
free_ctree_record(struct ctree_record* ctrec)
{
    ctdbFreeRecord(ctrec->handle);
    xfree(ctrec);
}

// Ctree::Record.new
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
                 ctdbGetError(&ctrec->table));

    rb_obj_call_init(obj, 0, NULL); // Ctree::Record.initialize
    return obj;
}

// Ctree::Record#default_index
static VALUE
ctree_record_get_default_index(VALUE obj)
{
    CTHANDLE *ct = GetCtreeRecord(obj)->table;
    return rb_str_new2(ctdbGetDefaultIndexName(ct));
}

// Ctree::Record#default_index=(name_or_index)
static VALUE
ctree_record_set_default_index(VALUE obj, VALUE name)
{
    // void *key;
    // 
    // switch(TYPE(name)){
    //     case T_STRING :
    //     case T_SYMBOL :
    //     case T_FIXNUM :
    //         
    //     default :
    //     
    // }
    return Qtrue;
}


// Ctree::Record#field(name)
// static VALUE
// ctree_record_get_value(VALUE obj, VALUE field)
// {
//     CTHANDLE *record = GetCtree(obj)->handle
// }

// Ctree::Session#set(field, value)
// static VALUE
// ctree_record_set_field(VALUE obj, VALUE name, VALUE value)
// {
//     struct ctree_record *ctrec = GetCtreeRecord(obj);
//     CTHANDLE *field;
// 
//     if((field = ctdGetFieldByName(ctrec->table, RSTRING_PTR(name))) == NULL)
//         rb_raise(cCtreeError, "[%d] ctdbGetFieldByName failed", 
//                  ctdbGetError(&ctrec->handle));
// 
//     switch(ctdbGetFieldType(field)){
//         case CT_CHAR :
//         case CT_CHARU :
//         case CT_STRING :
//         case CT_FSTRING :
//         case CT_FPSTRING :
//         case CT_F2STRING :
//         case CT_F4STRING :
//             // Check_Type(value, )
//             // do work
//             break;
//         case CT_CURRENCY :
//             
//             // do work
//             break;
//     }
// 
//     return Qtrue;
// }

// Ctree::Record#get(field)
// static VALUE
// ctree_record_get_field(VALUE obj, VALUE name)
// {
//     CTHANDLE ct = GetCtreeRecord(obj)->handle;
//     CTHANDLE *field;
//     CTDBRET rc;
//     int num, size;
//     void *cval;
//     VALUE rbval;
// 
//     if((num = ctdbGetFieldNumberByName(ct, RSTRING_PTR(name))) == -1)
//         rb_raise(cCtreeError, "[%d] ctdbGetFieldNumberByName failed.", 
//                  ctdbGetError(&ct));
// 
//     size = ctdbGetFieldDataLength(ct, num);
// 
//     char *value;
//     switch(ctdbGetFieldType(field)){
//         case CT_FSTRING :
//         case CT_FPSTRING :
//         case CT_F2STRING :
//         case CT_F4STRING :
//         case CT_PSTRING :
//         case CT_2STRING :
//         case CT_4STRING :
//         case CT_STRING :
//             if((rc = ctdbGetFieldAsString(ct, num, cval, size)) != CTDBRET_OK)
//                 rb_raise(cCtreeError, "[%d] ctdbGetFieldAsString failed.", rc);
// 
//             rb_str_new2();
//             break;
//         case CT_DATE :
//             rc = ctdbGetFieldAsDate(ct, num, cval);
//             break;
//         case CT_TIME : 
//             rc = ctdbGetFieldAsTime(ct, num, cval);
//             break;
//         case CT_UTINYINT :
//         case CT_CHARU :
//         case CT_USMALLINT :
//         case CT_INT2U :
//         case CT_UINTEGER :
//         case CT_INT4U :
//             rc = ctdbGetFieldAsUnsigned(ct, num, value);
//             break;
//         case CT_TINYINT :
//         case CT_CHAR :
//         case CT_SMALLINT :
//         case CT_INT2 :
//         case CT_INTEGER :
//         case CT_INT4 :
//             rc = ctdbGetFieldAsSigned(ct, num, value);
//             break;
//     }
//     if(rc != CTDBRET_OK)
//         
// 
//     return value;
// }

// Ctree::Record#write
static VALUE
ctree_record_write(VALUE obj)
{
    CTHANDLE *ct = GetCtree(obj)->handle;
    if(ctdbWriteRecord(ct))
        rb_raise(cCtreeError, "[%d] ctdbWriteRecord failed.", ctdbGetError(&ct));

    return Qtrue;
}

/*
 * Ctree::Field
 */
// static void
// free_ctree_field(struct ctree_field* ct)
// {
// }

void 
Init_ctree_sdk(void)
{
    mCtree = rb_define_module("Ctree");
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

    // Ctree::Error
    cCtreeError = rb_define_class_under(mCtree, "Error", rb_eStandardError);

    // Ctree::Session
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

    // Ctree::Table
    cCtreeTable = rb_define_class_under(mCtree, "Table", rb_cObject);
    rb_define_singleton_method(cCtreeTable, "new", ctree_table_init, 1);
    rb_define_method(cCtreeTable, "path=", ctree_table_set_path, 1);
    rb_define_method(cCtreeTable, "path", ctree_table_get_path, 0);
    rb_define_method(cCtreeTable, "name", ctree_table_get_name, 0);
    rb_define_method(cCtreeTable, "open", ctree_table_open, 1);
    rb_define_method(cCtreeTable, "close", ctree_table_close, 0);

    // Ctree::Record
    cCtreeRecord = rb_define_class_under(mCtree, "Record", rb_cObject);
    rb_define_singleton_method(cCtreeRecord, "new", ctree_record_init, 1);
    rb_define_method(cCtreeRecord, "default_index", ctree_record_get_default_index, 0);
    rb_define_method(cCtreeRecord, "default_index=", ctree_record_set_default_index, 1);
    
    // Ctree::Field
    // cCtreeField = rb_define_class_under(mCtree, "Field", rb_cObject);
    // rb_define_singleton_method(cCtreeRecord, "new", ctree_field_init, -1);
}