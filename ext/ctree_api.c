#include "ruby.h"
#include "ctdbsdk.h" /* c-tree headers */

#define CTREE_ENGINE "FAIRCOMS"
#define CTREE_USER "RUBY"
#define CTREE_PASS "YBUR"


VALUE cCtree;

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

static VALUE logon(int argc, VALUE* argv, VALUE klass)
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
    ctdbLogon(&ctp->handler, CTREE_ENGINE, CTREE_USER, CTREE_PASS);
}

static VALUE initialize(int argc, VALUE* argv, VALUE obj)
{
    return obj;
}

void Init_ctree_api(void)
{
    cCtree = rb_define_class("Ctree", rb_cObject);
    rb_define_singleton_method(cCtree, "init", init, 0);
    rb_define_singleton_method(cCtree, "logon", logon, -1);
    rb_define_method(cCtree, "initiallize", initialize, -1);
}