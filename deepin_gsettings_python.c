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
 *
 */

#include <Python.h>
#include <gio/gio.h>

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
} DeepinGSettingsObject;

static PyObject *m_deepin_gsettings_object_constants = NULL;
static PyTypeObject *m_DeepinGSettings_Type = NULL;

static DeepinGSettingsObject *m_init_deepin_gsettings_object();
static DeepinGSettingsObject *m_new(PyObject *self, PyObject *args);

static PyMethodDef deepin_gsettings_methods[] = 
{
    {"new", m_new, METH_VARARGS, "Deepin GSettings Construction"}, 
    {NULL, NULL, 0, NULL}
};

static char m_get_value_doc[] = "Gets the value that is stored at key in "
                                "settings";
static char m_set_value_doc[] = "Sets key in settings to value";

static PyObject *m_delete(DeepinGSettingsObject *self);
static PyObject *m_get_boolean(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_boolean(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_int(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_int(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_uint(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_uint(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_double(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_double(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_get_strv(DeepinGSettingsObject *self, PyObject *args);
static PyObject *m_set_strv(DeepinGSettingsObject *self, PyObject *args);

static PyMethodDef deepin_gsettings_object_methods[] = 
{
    {"delete", m_delete, METH_NOARGS, "Deepin GSettings Object Destruction"}, 
    {"get_boolean", m_get_boolean, METH_VARARGS, m_get_value_doc}, 
    {"set_boolean", m_set_boolean, METH_VARARGS, m_set_value_doc}, 
    {"get_int", m_get_int, METH_NOARGS, m_get_value_doc}, 
    {"set_int", m_set_int, METH_VARARGS, m_set_value_doc}, 
    {"get_uint", m_get_uint, METH_NOARGS, m_get_value_doc}, 
    {"set_uint", m_set_uint, METH_VARARGS, m_set_value_doc}, 
    {"get_double", m_get_double, METH_NOARGS, m_get_value_doc}, 
    {"set_double", m_set_double, METH_VARARGS, m_set_value_doc}, 
    {"get_strv", m_get_strv, METH_NOARGS, m_get_value_doc}, 
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
    if (v == NULL && dict1 != NULL)
        v = PyDict_GetItemString(dict1, name);
    if (v == NULL && dict2 != NULL)
        v = PyDict_GetItemString(dict2, name);
    if (v != NULL) {
        Py_INCREF(v);
        return v;
    }
    return Py_FindMethod(m, co, name);
}

static int m_setattr(PyObject **dict, char *name, PyObject *v)
{
    if (v == NULL) {
        int rv = -1;
        if (*dict != NULL)
            rv = PyDict_DelItemString(*dict, name);
        if (rv < 0) {
            PyErr_SetString(PyExc_AttributeError, 
                            "delete non-existing attribute");
            return rv;
        }
    }
    if (*dict == NULL) {
        *dict = PyDict_New();
        if (*dict == NULL)
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

    return self;
}

static DeepinGSettingsObject *m_new(PyObject *dummy, PyObject *args) 
{
    DeepinGSettingsObject *self = NULL;
    char *schema_id = NULL;
    
    self = m_init_deepin_gsettings_object();
    if (!self)
        return NULL;

    if (!PyArg_ParseTuple(args, "s", &schema_id))
        return NULL;

    g_type_init ();
    
    self->handle = g_settings_new(schema_id);

    return self;
}

static PyObject *m_delete(DeepinGSettingsObject *self) 
{
    if (self->handle) {
        g_object_unref(self->handle);
        self->handle = NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *m_get_boolean(DeepinGSettingsObject *self, PyObject *args) 
{
    char *key = NULL;

    if (!PyArg_ParseTuple(args, "s", &key))
        return Py_False;

    if (!self->handle)
        return Py_False;

    if (!g_settings_get_boolean(self->handle, key))
        return Py_False;

    return Py_True;
}

static PyObject *m_set_boolean(DeepinGSettingsObject *self, PyObject *args) 
{
    char *key = NULL;
    PyObject *value;

    if (!PyArg_ParseTuple(args, "sO", &key, &value))
        return Py_False;

    if (!self->handle)
        return Py_False;

    return Py_True;
}

static PyObject *m_get_int(DeepinGSettingsObject *self, PyObject *args) 
{
    char *key = NULL;



    return Py_True;
}

static PyObject *m_set_int(DeepinGSettingsObject *self, PyObject *args) 
{
    return Py_True;
}

static PyObject *m_get_uint(DeepinGSettingsObject *self, PyObject *args) 
{
    return Py_True;
}

static PyObject *m_set_uint(DeepinGSettingsObject *self, PyObject *args) 
{
    return Py_True;
}

static PyObject *m_get_double(DeepinGSettingsObject *self, PyObject *args) 
{
    return Py_True;
}

static PyObject *m_set_double(DeepinGSettingsObject *self, PyObject *args) 
{
    return Py_True;
}

static PyObject *m_get_strv(DeepinGSettingsObject *self, PyObject *args) 
{
    return Py_True;
}

static PyObject *m_set_strv(DeepinGSettingsObject *self, PyObject *args) 
{
    return Py_True;
}
