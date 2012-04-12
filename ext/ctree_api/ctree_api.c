#include "ruby.h"
#include "ctdbsdk.h" /* c-tree headers */

#define CTREE_ENGINE "FAIRCOMS"
#define CTREE_USER "RUBY"
#define CTREE_PASS "YBUR"


#define GetCtreeStruct(obj) (Check_Type(obj, T_DATA), (struct ctree*)DATA_PTR(obj))
#define GetHandler(obj)     (Check_Type(obj, T_DATA), &(((struct ctree*)DATA_PTR(obj))->handler))


VALUE cCtree;
VALUE cCtreeField;

struct ctree {
    CTHANDLE handler;
    char connection;
};

// free class object
static void free_ctree(struct ctree* ct)
{
    if(ct->connection == Qtrue)
        ctdbFreeSession(ct->handler);
    xfree(ct);
}

static VALUE init(VALUE klass)
{
    struct ctree* ctp;
    VALUE obj;

    obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree, ctp);
    ctp->handler = ctdbAllocSession(CTSESSION_CTREE);
    ctp->connection = Qfalse;
    rb_obj_call_init(obj, 0, NULL);
    return obj;
}

static VALUE connect(int argc, VALUE* argv, VALUE klass)
{
    VALUE host, username, password, database;
    char *h, *u, *p, *d;
    struct ctree* ctp;
    VALUE obj;

    rb_scan_args(argc, argv, "04", &host, &username, &password, &database);
    // h = NILorSTRING(host);
    // u = NILorSTRING(username);
    // p = NILorSTRING(password);
    // d = NILorSTRING(database);

    obj = Data_Make_Struct(klass, struct ctree, 0, free_ctree, ctp);
    // ctdbLogon(&ctp->handler, CTREE_ENGINE, CTREE_USER, CTREE_PASS);
    ctdbConnect(&ctp->handler, CTREE_ENGINE);
}

static VALUE disconnect(VALUE obj)
{
    CTHANDLE* cth = GetHandler(obj);
    ctdbDisconnect(cth);
    GetCtreeStruct(obj)->connection = Qfalse;
    return obj;
}

static VALUE initialize(int argc, VALUE* argv, VALUE obj)
{
    return obj;
}

void Init_ctree_api(void)
{
    cCtree = rb_define_class("Ctree", rb_cObject);
    cCtreeField = rb_define_class_under(cCtree, "Field", rb_cObject);

    // class methods
    rb_define_singleton_method(cCtree, "init", init, 0);
    rb_define_singleton_method(cCtree, "connect", connect, -1);
    // instance methods
    rb_define_method(cCtree, "disconnect", disconnect, 0);
    rb_define_method(cCtree, "initiallize", initialize, -1);

    // rb_define_method(cCtreeField, "name", field_name, 0);
    // rb_define_method(cCtreeField, "table", field_table, 0);
    // rb_define_method(cCtreeField, "type", field_type, 0);
    // rb_define_method(cCtreeField, "length", field_length, 0);
}