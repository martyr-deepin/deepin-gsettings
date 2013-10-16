/* 
 * Copyright (C) 2012 Deepin, Inc.
 *               2012 Zhai Xiang
 *
 * Author:     Zhai Xiang <zhaixiang@linuxdeepin.com>
 * Maintainer: Zhai Xiang <zhaixiang@linuxdeepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Python.h>
#include <gio/gio.h>

#define INT(v) PyInt_FromLong(v)
#define DOUBLE(v) PyFloat_FromDouble(v)
#define ERROR(v) PyErr_SetString(PyExc_TypeError, v)

/* Safe XDECREF for object states that handles nested deallocations */
#define ZAP(v) do {\
    PyObject *tmp = (PyObject *)(v); \
    (v) = NULL; \
    Py_XDECREF(tmp); \
} while (0)

typedef struct {
    PyObject_HEAD
    PyObject *dict; /* Python attributes dictionary */
    GSettings *handle;
    PyObject *changed_cb;
} DeepinGSettingsObject;

static PyObject *m_deepin_gsettings_object_constants = NULL;
static PyTypeObject *m_DeepinGSettings_Type = NULL;

static DeepinGSettingsObject *m_init_deepin_gsettings_object();
static DeepinGSettingsObject *m_new(PyObject *self, PyObject *args);
static DeepinGSettingsObject *m_new_with_path(PyObject *self, PyObject *args);
static void m_changed_cb(GSettings *settings, gchar *key, gpointer user_data);

static PyMethodDef deepin_gsettings_methods[] = 
{
    {"new", m_new, METH_VARARGS, "Deepin GSettings Construction"}, 
    {"new_with_path", m_new_with_path, METH_VARARGS, "Deepin GSettings Construction with path"}, 
    {NULL, NULL, 0, NULL}
};

static char m_get_value_doc[] = "Gets the value that is stored at key in "
                                "settings";
static char m_set_value_doc[] = "Sets key in settings to value";

static PyObject *m_delete(DeepinGSettingsObject *self);
static PyObject *m_connect(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_reset(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_list_keys(DeepinGSettingsObject *self);
static PyObject *m_get_boolean(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_boolean(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_int(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_int(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_uint(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_uint(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_double(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_double(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_string(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_string(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_strv(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_strv(DeepinGSettingsObject *self, PyObject *args);

static PyMethodDef deepin_gsettings_object_methods[] = 
{
    {"delete", m_delete, METH_NOARGS, "Deepin GSettings Object Destruction"}, 
    {"connect", m_connect, METH_VARARGS, "signal response"}, 
    {"reset", m_reset, METH_VARARGS, "Resets key to its default value"}, 
    {"list_keys", m_list_keys, METH_NOARGS, 
     "Introspects the list of keys on settings"}, 
    {"get_boolean", m_get_boolean, METH_VARARGS, m_get_value_doc}, 
    {"set_boolean", m_set_boolean, METH_VARARGS, m_set_value_doc}, 
    {"get_int", m_get_int, METH_VARARGS, m_get_value_doc}, 
    {"set_int", m_set_int, METH_VARARGS, m_set_value_doc}, 
    {"get_uint", m_get_uint, METH_VARARGS, m_get_value_doc}, 
    {"set_uint", m_set_uint, METH_VARARGS, m_set_value_doc}, 
    {"get_double", m_get_double, METH_VARARGS, m_get_value_doc}, 
    {"set_double", m_set_double, METH_VARARGS, m_set_value_doc}, 
    {"get_string", m_get_string, METH_VARARGS, m_get_value_doc}, 
    {"set_string", m_set_string, METH_VARARGS, m_set_value_doc}, 
    {"get_strv", m_get_strv, METH_VARARGS, m_get_value_doc}, 
    {"set_strv", m_set_strv, METH_VARARGS, m_set_value_doc}, 
    {NULL, NULL, 0, NULL}
};

static void m_deepin_gsettings_dealloc(DeepinGSettingsObject *self) 
{
    PyObject_GC_UnTrack(self);
    Py_TRASHCAN_SAFE_BEGIN(self)

    ZAP(self->dict);
    m_delete(self);

    PyObject_GC_Del(self);
    Py_TRASHCAN_SAFE_END(self)
}

static PyObject *m_getattr(PyObject *co, 
                           char *name, 
                           PyObject *dict1, 
                           PyObject *dict2, 
                           PyMethodDef *m)
{
    PyObject *v = NULL;
    
    if (dict1)
        v = PyDict_GetItemString(dict1, name);
    if (!v && dict2)
        v = PyDict_GetItemString(dict2, name);
    if (v) {
        Py_INCREF(v);
        return v;
    }
    
    return Py_FindMethod(m, co, name);
}

static int m_setattr(PyObject **dict, char *name, PyObject *v)
{
    if (!v) {
        int rv = -1;
        if (*dict)
            rv = PyDict_DelItemString(*dict, name);
        if (rv < 0) {
            PyErr_SetString(PyExc_AttributeError, 
                            "delete non-existing attribute");
            return rv;
        }
    }
    if (!*dict) {
        *dict = PyDict_New();
        if (!*dict)
            return -1;
    }
    return PyDict_SetItemString(*dict, name, v);
}

static PyObject *m_deepin_gsettings_getattr(DeepinGSettingsObject *dgo, 
                                            char *name) 
{
    return m_getattr((PyObject *)dgo, 
                     name, 
                     dgo->dict, 
                     m_deepin_gsettings_object_constants, 
                     deepin_gsettings_object_methods);
}

static PyObject *m_deepin_gsettings_setattr(DeepinGSettingsObject *dgo, 
                                            char *name, 
                                            PyObject *v) 
{
    return m_setattr(&dgo->dict, name, v);
}

static PyObject *m_deepin_gsettings_traverse(DeepinGSettingsObject *self, 
                                             visitproc visit, 
                                             void *args) 
{
    int err;
#undef VISIT
#define VISIT(v)    if ((v) != NULL && ((err = visit(v, args)) != 0)) return err

    VISIT(self->dict);

    return 0;
#undef VISIT
}

static PyObject *m_deepin_gsettings_clear(DeepinGSettingsObject *self) 
{
    ZAP(self->dict);
    return 0;
}

static PyTypeObject DeepinGSettings_Type = {
    PyObject_HEAD_INIT(NULL)
    0, 
    "deepin_gsettings.new", 
    sizeof(DeepinGSettingsObject), 
    0, 
    (destructor)m_deepin_gsettings_dealloc,
    0, 
    (getattrfunc)m_deepin_gsettings_getattr, 
    (setattrfunc)m_deepin_gsettings_setattr, 
    0, 
    0, 
    0,  
    0,  
    0,  
    0,  
    0,  
    0,  
    0,  
    0,  
    Py_TPFLAGS_HAVE_GC,
    0,  
    (traverseproc)m_deepin_gsettings_traverse, 
    (inquiry)m_deepin_gsettings_clear
};

PyMODINIT_FUNC initdeepin_gsettings() 
{
    PyObject *m = NULL;
             
    m_DeepinGSettings_Type = &DeepinGSettings_Type;
    DeepinGSettings_Type.ob_type = &PyType_Type;

    m = Py_InitModule("deepin_gsettings", deepin_gsettings_methods);
    if (!m)
        return;

    m_deepin_gsettings_object_constants = PyDict_New();
}

static DeepinGSettingsObject *m_init_deepin_gsettings_object() 
{
    DeepinGSettingsObject *self = NULL;

    self = (DeepinGSettingsObject *) PyObject_GC_New(DeepinGSettingsObject, 
                                                     m_DeepinGSettings_Type);
    if (!self)
        return NULL;
    PyObject_GC_Track(self);

    self->dict = NULL;
    self->handle = NULL;
    self->changed_cb = NULL;

    return self;
}

/* TODO: g_signal_connect work in the g_mainloop thread
 *       and there is also Python looping thread 
 */
static void m_changed_cb(GSettings *settings, gchar *key, gpointer user_data) 
{
    DeepinGSettingsObject *self = (DeepinGSettingsObject *) user_data;
    PyGILState_STATE gstate;

    if (!self->changed_cb) 
        return;
        
    /* 
     * TODO: Thanks super _synh_
     *       Thread mutex lock Python Style 
     */
    gstate = PyGILState_Ensure();
    PyEval_CallFunction(self->changed_cb, "(s)", key);
    PyGILState_Release(gstate);
}

static DeepinGSettingsObject *m_new(PyObject *dummy, PyObject *args) 
{
    DeepinGSettingsObject *self = NULL;
    gchar *schema_id = NULL;

    if (self) 
        return self;

    self = m_init_deepin_gsettings_object();
    if (!self)
        return NULL;

    if (!PyArg_ParseTuple(args, "s", &schema_id))
        return NULL;

    self->handle = g_settings_new(schema_id);
    if (!self->handle) {
        ERROR("g_settings_new error");
        m_delete(self);
        return NULL;
    }

    g_signal_connect(self->handle, "changed", G_CALLBACK(m_changed_cb), self);

    return self;
}

static DeepinGSettingsObject *m_new_with_path(PyObject *dummy, PyObject *args)            
{                                                                               
    DeepinGSettingsObject *self = NULL;                                         
    gchar *schema_id = NULL;
    gchar *path = NULL;    

    if (self) 
        return self;

    self = m_init_deepin_gsettings_object();                                    
    if (!self)                                                                  
        return NULL;                                                            

    if (!PyArg_ParseTuple(args, "ss", &schema_id, &path))                               
        return NULL;                                                            

    self->handle = g_settings_new_with_path(schema_id, path);                                   
    if (!self->handle) {                                                        
        ERROR("g_settings_new_with_path error");                                          
        m_delete(self);                                                         
        return NULL;                                                            
    }                                                                           

    g_signal_connect(self->handle, "changed", G_CALLBACK(m_changed_cb), self);  

    return self;
}

static PyObject *m_delete(DeepinGSettingsObject *self) 
{
    if (self->handle) {
        g_object_unref(self->handle);
        self->handle = NULL;
    }

    self->changed_cb = NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *m_connect(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *name = NULL;
    PyObject *fptr = NULL;

    if (!PyArg_ParseTuple(args, "sO:set_callback", &name, &fptr)) { 
        ERROR("invalid arguments to connect");
        return NULL;
    }

    if (!PyCallable_Check(fptr)) {
        Py_INCREF(Py_False);
        return Py_False;
    }
    
    if (strcmp(name, "changed") == 0) { 
        Py_XINCREF(fptr);
        Py_XDECREF(self->changed_cb);
        self->changed_cb = fptr;
    }

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *m_reset(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;

    if (!PyArg_ParseTuple(args, "s", &key)) {                                   
        Py_INCREF(Py_False);                                                    
        return Py_False;                                                        
    }

    g_settings_reset(self->handle, key);

    Py_INCREF(Py_True);                                                         
    return Py_True;        
}

static PyObject *m_list_keys(DeepinGSettingsObject *self) 
{
    gchar** keys = g_settings_list_keys(self->handle);
    int len = g_strv_length(keys);
    PyObject* list = PyList_New(len);
    int i=0;
    for (; i<len; i++) {
        PyList_SetItem(list, i, PyString_FromString(keys[i]));
    }
    g_strfreev(keys);
    return list;
}

/* TIP: Please do not directly return Py_True, call Py_INCREF(Py_True) at first
 *      Python GC will free Py_True when reference counting is 0
 */
static PyObject *m_get_boolean(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;

    if (!PyArg_ParseTuple(args, "s", &key)) {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (!g_settings_get_boolean(self->handle, key)) {
        Py_INCREF(Py_False);
        return Py_False;
    }

    Py_INCREF(Py_True);
    return Py_True;
}

/* TODO: When set_XXX, it need to check the variable data type, for example, 
 *       PyBool_Check()
 */
static PyObject *m_set_boolean(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;
    PyObject *value = NULL;

    if (!PyArg_ParseTuple(args, "sO", &key, &value)) {
        Py_INCREF(Py_False);
        return Py_False;
    }

    if (!PyBool_Check(value)) {
        Py_INCREF(Py_False);
        return Py_False;
    }
    
    if (value == Py_True) 
        g_settings_set_boolean(self->handle, key, 1);
    else
        g_settings_set_boolean(self->handle, key, 0);
    g_settings_sync();

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *m_get_int(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;

    if (!PyArg_ParseTuple(args, "s", &key)) {
        ERROR("invalid arguments to get_int");
        return NULL;
    }
    
    return INT(g_settings_get_int(self->handle, key));
}

static PyObject *m_set_int(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;
    gint value = -1;

    if (!PyArg_ParseTuple(args, "si", &key, &value)) { 
        ERROR("invalid arguments to set_int");
        return NULL;
    }

    if (!g_settings_set_int(self->handle, key, value)) { 
        Py_INCREF(Py_False);
        return Py_False;
    }
    g_settings_sync();
    
    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *m_get_uint(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;

    if (!PyArg_ParseTuple(args, "s", &key)) 
        return INT(0);

    if (!self->handle)
        return INT(0);

    return INT(g_settings_get_uint(self->handle, key));
}

static PyObject *m_set_uint(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;
    unsigned int value = 0;

    if (!PyArg_ParseTuple(args, "sI", &key, &value)) { 
        ERROR("invalid arguments to set_uint");
        return NULL;
    }

    if (!g_settings_set_uint(self->handle, key, value)) { 
        Py_INCREF(Py_False);
        return Py_False;
    }
    g_settings_sync();
    
    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *m_get_double(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;

    if (!PyArg_ParseTuple(args, "s", &key)) {
        ERROR("invalid arguments to get_double");
        return NULL;
    }

    return DOUBLE(g_settings_get_double(self->handle, key));
}

static PyObject *m_set_double(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;
    gdouble value = 0.0;

    if (!PyArg_ParseTuple(args, "sd", &key, &value)) { 
        ERROR("invalid arguments to set_double");
        return NULL;
    }

    if (!g_settings_set_double(self->handle, key, value)) { 
        Py_INCREF(Py_False);
        return Py_False;
    }
    g_settings_sync();

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *m_get_string(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;

    if (!PyArg_ParseTuple(args, "s", &key)) { 
        ERROR("invalid arguments to get_string");
        return NULL;
    }

    gchar* str = g_settings_get_string(self->handle, key);
    PyObject* ret = PyString_FromString(str);
    g_free(str);
    return ret;
}

static PyObject *m_set_string(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;
    gchar *value = NULL;

    if (!PyArg_ParseTuple(args, "ss", &key, &value)) { 
        ERROR("invalid arguments to set_string");
        return NULL;
    }

    if (!g_settings_set_string(self->handle, key, value)) { 
        Py_INCREF(Py_False);
        return Py_False;
    }
    g_settings_sync();

    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject *m_get_strv(DeepinGSettingsObject *self, PyObject *args) 
{
    char* key = NULL;
    if (!PyArg_ParseTuple(args, "s", &key)) { 
        ERROR("invalid arguments to get_strv");
        return NULL;
    }

    gchar** strv = g_settings_get_strv(self->handle, key);
    int len = g_strv_length(strv);
    PyObject *list = PyList_New(len);
    int i=0;
    for (; i<len; i++) {
        PyList_SetItem(list, i, PyString_FromString(strv[i]));
    }
    g_strfreev(strv);
    return list;
}

static PyObject *m_set_strv(DeepinGSettingsObject *self, PyObject *args) 
{
    gchar *key = NULL;
    PyObject *value = NULL;
    char *item_str = NULL;
    int item_length = 0;
    gchar **strv;
    gsize length = 0;
    int i;

    if (!PyArg_ParseTuple(args, "sO", &key, &value)) {
        ERROR("invalid arguments to set_strv");
        return NULL;
    }
    
    if (!PyList_Check(value)) {
        Py_INCREF(Py_False);
        return Py_False;
    }
    
    length = PyList_Size(value);
    /* TODO: Allocation length + 1 for GSettings gchar** array 
     *       Because there is no length argv any more 
     */
    strv = malloc((length + 1) * sizeof(gchar *));
    if (!strv) {
        Py_INCREF(Py_False);
        return Py_False;
    }
    memset(strv, 0, (length + 1) * sizeof(gchar *));
    
    for (i = 0; i < length; i++) { 
        item_str = PyString_AsString(PyList_GetItem(value, i));
        item_length = (strlen(item_str) + 1) * sizeof(gchar);
        strv[i] = malloc(item_length);
        if (!strv[i]) { 
            Py_INCREF(Py_False);
            return Py_False;
        }
        memset(strv[i], 0, item_length);
        strcpy(strv[i], item_str);
    }
    /* TODO: Tell GSettings the gchar** array size */
    strv[length] = NULL;

    if (!g_settings_set_strv(self->handle, key, strv)) {
        g_strfreev(strv);
        Py_INCREF(Py_False);
        return Py_False;
    }
    g_settings_sync();

    g_strfreev(strv);

    Py_INCREF(Py_True);
    return Py_True;
}
