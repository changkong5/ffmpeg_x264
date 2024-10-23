#include <db.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "initcall.h"
#include "gdbus/gdbus.h"

typedef struct customer {
    int  c_id;
    char name[10];
    char address[20];
    int  age;
} CUSTOMER;

void init_DBT(DBT * key, DBT * data)
{
    memset(key, 0, sizeof(DBT));
    memset(data, 0, sizeof(DBT));
}

int main111(void)
{
    DB_ENV *dbenv;
    DB *dbp;    
    DBT key, data;

    int ret = 0;
    int key_cust_c_id = 1;
    CUSTOMER cust = {1, "chenqi", "beijing", 30}; 

    /* initialize env handler */
    if (ret = db_env_create(&dbenv, 0)) { 
        printf("db_env_create ERROR: %s\n", db_strerror(ret));
        goto failed;
    }   

    u_int32_t flags = DB_CREATE | DB_INIT_MPOOL | DB_INIT_CDB | DB_THREAD;;  

    // mkdir /root/libdb
    if (ret = dbenv->open(dbenv, "/root/libdb", flags, 0)) {
        printf("dbenv->open ERROR: %s\n", db_strerror(ret));
        goto failed;
    }   

    /* initialize db handler */
    if (ret = db_create(&dbp, dbenv, 0)) {
        printf("db_create ERROR: %s\n", db_strerror(ret));
        goto failed;
    }   

    flags = DB_CREATE | DB_THREAD;

    if (ret = dbp->open(dbp, NULL, "single.db", NULL, DB_BTREE, flags, 0664)) {
        printf("dbp->open ERROR: %s\n", db_strerror(ret));
        goto failed;
    }

    /* write record */
    /* initialize DBT */
    init_DBT(&key, &data);
    key.data = &key_cust_c_id;
    key.size = sizeof(key_cust_c_id);
    data.data = &cust;
    data.size = sizeof(CUSTOMER);

    // if (ret = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE)) {
    if (ret = dbp->put(dbp, NULL, &key, &data, DB_OVERWRITE_DUP)) {
        printf("dbp->put ERROR: %s\n", db_strerror(ret));
        goto failed;
    }

    /* flush to disk */
    dbp->sync(dbp, 0);

    /* get record */
    init_DBT(&key, &data);
    key.data = &key_cust_c_id;
    key.size = sizeof(key_cust_c_id);
    data.flags = DB_DBT_MALLOC;

    if (ret = dbp->get(dbp, NULL, &key, &data, 0)) {
        printf("dbp->get ERROR: %s\n", db_strerror(ret));
        goto failed;
    }

    CUSTOMER *info = data.data;

    printf("id = %d\nname=%s\naddress=%s\nage=%d\n",
            info->c_id,
            info->name,
            info->address,
            info->age);

    /* free */
    free(data.data);

    if(dbp) {
        dbp->close(dbp, 0);
    }

    if (dbenv) {
    	dbenv->close(dbenv, 0);
    }

    return 0;
    
failed:
    if(dbp) {
        dbp->close(dbp, 0);
    }

    if (dbenv) {
        dbenv->close(dbenv, 0);
    }

    return -1;
}

static gboolean cancel_fire_db(gpointer data){
  g_print("cancel_fire_db() quit \n");

  main111();

  return G_SOURCE_REMOVE;
}

static int main_init(void)
{
	printf("-----main_init------ \n");

	// g_timeout_add(5000, cancel_fire_db, NULL);
	/* 
	g_timeout_add(3000, cancel_fire_8, NULL);
	g_timeout_add(5000, cancel_fire_9, NULL);
	g_timeout_add(8000, cancel_fire_a, NULL);
	g_timeout_add(15000, cancel_fire_b, NULL);
	*/
	return 0;
}
fn_initcall(main_init);

