#include "ruby.h"
#include "ctdbsdk.h" /* c-tree headers */

#define NILorSTRING(obj) ( NIL_P(obj) ? NULL : StringValuePtr(obj) )

#define GetCtreeSession(obj) (Check_Type(obj, T_DATA), (struct ctree_session*)DATA_PTR(obj))
#define GetCtreeTable(obj)   (Check_Type(obj, T_DATA), (struct ctree_table*)DATA_PTR(obj))

VALUE mCtree;
VALUE cCtreeError;    // Ctree::Error
VALUE cCtreeSession;  // Ctree::Session
VALUE cCtreeTable;    // Ctree::Table
VALUE cCtreeRecord;   // Ctree::Record
VALUE cCtreeField;    // Ctree::Field

struct ctree_session {
    CTHANDLE handle;
};

struct ctree_table {
    CTHANDLE handle;
};

struct ctree_record {
    CTHANDLE handle;
};

struct ctree_field {
    CTHANDLE handle;
};

/*
 * Ctree::Session
 */
static void 
free_ctree_session(struct ctree_session* ct)
{
    ctdbFreeSession(ct->handle);
    xfree(ct);
}

// Ctree::Session.new
static VALUE
ctree_session_init(VALUE klass)
{
    struct ctree_session* ct;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree_session, 0, free_ctree_session, ct);

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
    char *h, *u, *p;
    CTHANDLE* session = GetCtreeSession(obj)->handle;

    rb_scan_args(argc, argv, "03", &host, &user, &pass);
    h = NILorSTRING(host);
    u = NILorSTRING(user);
    p = NILorSTRING(pass);

    if(ctdbLogon(session, h, u, p) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbLogon failed.", ctdbGetError(&session));

    return obj;
}

// Ctree::Session#logout
static VALUE 
ctree_session_logout(VALUE obj)
{
    CTHANDLE* session = GetCtreeSession(obj)->handle;
    ctdbLogout(session);
    return obj;
}

// Ctree::Session#active?
static VALUE 
ctree_session_active(VALUE obj)
{
    CTHANDLE* session = GetCtreeSession(obj)->handle;
    return ctdbIsActiveSession(session) ? Qtrue : Qfalse;
}

/*
 * Ctree::Table
 */
static void
free_ctree_table(struct ctree_table* ct)
{
    ctdbFreeTable(ct->handle);
    xfree(ct);
}

// Ctree::Table.new
static VALUE
ctree_table_init(VALUE klass, VALUE session)
{
    Check_Type(session, T_DATA);

    struct ctree_table* ct;
    CTHANDLE *cth = GetCtreeSession(session)->handle;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree_table, 0, free_ctree_table, ct);

    if((ct->handle = ctdbAllocTable(cth)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbAllocTable failed", ctdbGetError(&cth));

    rb_obj_call_init(obj, 0, NULL); // Ctree::Table.initialize
    return obj;
}

// Ctree::Table#path=(value)
static VALUE
ctree_table_path_set(VALUE obj, VALUE path)
{
    Check_Type(path, T_STRING);

    CTHANDLE *ct = GetCtreeTable(obj)->handle;

    if(ctdbSetTablePath(ct, RSTRING_PTR(path)) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbSetTablePath failed.", ctdbGetError(&ct));

    return Qnil;
}

// Ctree::Table#path
static VALUE
ctree_table_path_get(VALUE obj)
{
    VALUE s = rb_str_new2(ctdbGetTablePath(GetCtreeTable(obj)->handle));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

// Ctree::Table#name
static VALUE
ctree_table_name_get(VALUE obj)
{
    VALUE s = rb_str_new2(ctdbGetTableName(GetCtreeTable(obj)->handle));
    return RSTRING_LEN(s) == 0 ? Qnil : s;
}

// Ctree::Table#open(name)
static VALUE
ctree_table_open(VALUE obj, VALUE name)
{
    Check_Type(name, T_STRING);

    CTHANDLE *ct = GetCtreeSession(obj)->handle;

    if(ctdbOpenTable(ct, "custmast", CTOPEN_NORMAL) !=  CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbOpenTable failed.", ctdbGetError(&ct));

    return obj;
}

// Ctree::Table#close
static VALUE
ctree_table_close(VALUE obj)
{
    CTHANDLE *ct = GetCtreeTable(obj)->handle;

    if(ctdbCloseTable(ct) != CTDBRET_OK)
        rb_raise(cCtreeError, "[%d] ctdbCloseTable failed.", ctdbGetError(&ct));

    return Qtrue;
}

/*
 * Ctree::Record
 */
static void 
free_ctree_record(struct ctree_record* ct)
{
    ctdbFreeRecord(ct->handle);
    xfree(ct);
}
static VALUE
ctree_record_init(VALUE klass, VALUE table)
{
    Check_Type(table, T_DATA);

    struct ctree_record* ct;
    CTHANDLE *cth = GetCtreeTable(table)->handle;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree_record, 0, free_ctree_record, ct);
    if((ct->handle = ctdbAllocRecord(cth)) == NULL)
        rb_raise(cCtreeError, "[%d] ctdbAllocRecord failed.", ctdbGetError(&cth));

    rb_obj_call_init(obj, 0, NULL); // Ctree::Record.initialize
    return obj;
}

// Ctree::Record#field(name)
// static VALUE
// ctree_record_get_field(VALUE obj,)
// {
//     
// }

// static VALUE
// ctree_record_set_field()
// {
//     
// }

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

    // Ctree::Error
    cCtreeError = rb_define_class_under(mCtree, "Error", rb_eStandardError);

    // Ctree::Session
    cCtreeSession = rb_define_class_under(mCtree, "Session", rb_cObject);
    rb_define_singleton_method(cCtreeSession, "new", ctree_session_init, 0);
    rb_define_method(cCtreeSession, "logon", ctree_session_logon, -1);
    rb_define_method(cCtreeSession, "logout", ctree_session_logout, 0);
    rb_define_method(cCtreeSession, "active?", ctree_session_active, 0);

    // Ctree::Table
    cCtreeTable = rb_define_class_under(mCtree, "Table", rb_cObject);
    rb_define_singleton_method(cCtreeTable, "new", ctree_table_init, 1);
    rb_define_method(cCtreeTable, "path=", ctree_table_path_set, 1);
    rb_define_method(cCtreeTable, "path", ctree_table_path_get, 0);
    rb_define_method(cCtreeTable, "name", ctree_table_name_get, 0);
    rb_define_method(cCtreeTable, "open", ctree_table_open, 1);
    rb_define_method(cCtreeTable, "close", ctree_table_close, 0);

    // Ctree::Record
    cCtreeRecord = rb_define_class_under(mCtree, "Record", rb_cObject);
    rb_define_singleton_method(cCtreeRecord, "new", ctree_session_init, 0);

    // Ctree::Field
    // cCtreeField = rb_define_class_under(mCtree, "Field", rb_cObject);
    // rb_define_singleton_method(cCtreeRecord, "new", ctree_field_init, -1);
}