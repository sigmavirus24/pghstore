#include <Python.h>
#include <stdio.h>
#include <string.h>

#define CHAR_CHECK(s, pos, c)                        \
  if (s[pos] != char) {                              \
    PyErr_Format(PyExc_ValueError,                   \
                 "Encountered %c at %i, was expecting %c.",   \
                 s[pos], pos, c);                             \
    return NULL;                                              \
  }

#define isEven(a) (((a) & 1) == 0)

#define ConcatOrGotoCleanup(result, string) { PyString_Concat(&result, string); if (!result) { goto _speedup_dumps_cleanup_and_exit; } }

/*
 * Just like strchr, but find first -unescaped- occurrence of c in s.
 */
char *
strchr_unescaped(char *s, char c) 
{
  char *p = strchr(s, c), *q;
  while (p != NULL) { /* loop through all the c's */
    q = p; /* scan backwards through preceding escapes */
    while (q > s && *(q-1) == '\\')
      q--;
    if (isEven(p-q)) /* even number of esc's => c is good */
        return p;
    p = strchr(p+1, c); /* else odd escapes => c is escaped, keep going */
  }
  return NULL;
}

static PyObject *
_speedups_loads(PyObject *self, PyObject *args, PyObject *keywds)
{
  static char *keyword_argument_names[] = {"encoding", "return_type", NULL};
  const char *errors = "strict";
  char *encoding = "utf-8";
  char *s;
  int i, null_value = 0;
  char *p[4];
  PyTypeObject *return_type = &PyDict_Type;
  PyObject *return_value, *encoded_key, *encoded_value, *key, *value;

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|sO:set_callback", keyword_argument_names, &s, &encoding, &return_type)) {
    return NULL;
  }

  return_value = PyType_GenericNew(return_type, PyTuple_New(0), PyDict_New());
  
  // Each iteration will find one key/value pair
  while ((p[0] = strchr(s, '"')) != NULL) {
    // move to the next character after the found "
    p[0]++;

    // Find end of key
    p[1] = strchr_unescaped(p[0], '"');

    // Find begining of value or NULL
    for (i=1; 1; i++) {
      switch (*(p[1]+i)) {
      case 'N':
      case 'n':
        // found NULL
        null_value = 1;
        break;
      case '"':
        // found begining of value
        p[2] = p[1]+i+1;
        break;
      case '\0':
        // found end of string
        return return_value;
      default:
        // neither NULL nor begining of value, keep looking
        continue;
      }
      break;
    }
     
    encoded_key = PyString_FromStringAndSize(p[0], p[1]-p[0]);
    key = PyUnicode_FromEncodedObject(encoded_key, encoding, errors);
    Py_DECREF(encoded_key);
    if (null_value == 0) {
      // find and null terminate end of value
      p[3] = strchr_unescaped(p[2], '"');
      encoded_value = PyString_FromStringAndSize(p[2], p[3] - p[2]);
      value = PyUnicode_FromEncodedObject(encoded_value, encoding, errors);
      Py_DECREF(encoded_value);
    } else {
      Py_INCREF(Py_None);
      value = Py_None;
    }
    if (PyDict_Check(return_value)) {
      PyDict_SetItem(return_value, key, value);
    } else {
      PyList_Append(return_value, PyTuple_Pack(2, key, value));
    }
    Py_DECREF(key);
    Py_DECREF(value);
    
    // set new search position
    if (null_value == 0) {
      s = p[3]+1;
    } else {
      s = p[1]+i;
    }

    // reset null value flag
    null_value = 0;
  }
  return return_value;
}

#define COMMA ","
#define ARROW "=>"
#define EMPTY ""
#define S_NULL "NULL"
#define CITATION "\""

static PyObject *
_speedups_dumps(PyObject *self, PyObject *args, PyObject *keywds)
{
  static char *keyword_argument_names[] = {"obj", "key_map", "value_map", "encoding", "return_unicode", NULL};
  int i = 0;
  const char *errors = "strict";
  char *encoding = "utf-8";
  PyObject *obj; //, *list;
  PyObject *return_unicode = Py_False;
  PyObject *key_map_callback, *value_map_callback = NULL;
  PyObject *unencoded_key, *key, *unencoded_value, *value, *result;
  PyObject *comma, *arrow, *empty, *s_null, *citation;
  Py_ssize_t list_len, pos = 0;
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOsO", keyword_argument_names, &obj, &key_map_callback, &value_map_callback, &encoding, &return_unicode)) {
    return NULL;
  }

  // return empty string if we got an empty dict
  empty = PyString_FromString(EMPTY);
  list_len = PyDict_Size(obj)*8-1;
  if (list_len == -1) {
    return empty;
  }
  
  // create string constants
  comma = PyString_FromString(COMMA);
  arrow = PyString_FromString(ARROW);
  s_null = PyString_FromString(S_NULL);
  citation = PyString_FromString(CITATION);
  
  result = PyString_FromString(EMPTY);
  //list = PyList_New(list_len);

  while (PyDict_Next(obj, &pos, &unencoded_key, &unencoded_value)) {
    // add comma (,)
    if (i > 0) {
      // Py_INCREF(comma);
      // PyList_SetItem(list, i, comma); i++;
      ConcatOrGotoCleanup(result, comma);
      printf("Concatenated a comma\n");
    }
    // add key
    // Py_INCREF(citation);
    // PyList_SetItem(list, i, citation); i++;
    ConcatOrGotoCleanup(result, citation);
    printf("Concatenated a dquote\n");
    if (PyUnicode_Check(unencoded_key)) {
        key = PyUnicode_AsEncodedString(unencoded_key, encoding, errors);
    } else {
        key = PyObject_Str(unencoded_key);
    }
    if (PyCallable_Check(key_map_callback)) {
        key = PyObject_Call(key_map_callback, PyTuple_Pack(1, key), PyDict_New());
    }

    if (PyUnicode_Check(unencoded_value)) {
        value = PyUnicode_AsEncodedString(unencoded_value, encoding, errors);
    } else {
        value = unencoded_value;
    }
    if (PyCallable_Check(value_map_callback)) {
        value = PyObject_Call(value_map_callback, PyTuple_Pack(1, value), PyDict_New());
    }
    // PyList_SetItem(list, i, PyObject_Str(key)); i++;
    // Py_INCREF(citation);
    // PyList_SetItem(list, i, citation); i++;
    printf("Concatenating the key\n");
    ConcatOrGotoCleanup(result, key);
    printf("Concatenated the key\n");
    ConcatOrGotoCleanup(result, citation);
    printf("Concatenated a dquote\n");
    // add arrow (=>)
    // Py_INCREF(arrow);
    // PyList_SetItem(list, i, arrow); i++;
    ConcatOrGotoCleanup(result, arrow);
    printf("Concatenated the hashrocket\n");
    // add value or null
    if (value != Py_None) {
      // add value
      // Py_INCREF(citation);
      // PyList_SetItem(list, i, citation); i++;
      // PyList_SetItem(list, i, PyObject_Str(value)); i++;
      // Py_INCREF(citation);
      // PyList_SetItem(list, i, citation); i++;
      ConcatOrGotoCleanup(result, citation);
      ConcatOrGotoCleanup(result, PyObject_Str(value));
      ConcatOrGotoCleanup(result, citation);
      printf("Concatenated the dquoted value\n");
    } else {
      // add null
      // Py_INCREF(empty);
      // PyList_SetItem(list, i, empty); i++;
      // Py_INCREF(s_null);
      // PyList_SetItem(list, i, s_null); i++;
      // Py_INCREF(empty);
      // PyList_SetItem(list, i, empty); i++;
      ConcatOrGotoCleanup(result, empty);
      ConcatOrGotoCleanup(result, s_null);
      ConcatOrGotoCleanup(result, empty);
      printf("Concatenated NULL\n");
    }
    i++;
  }
  // result = PyObject_CallMethod(empty, "join", "O", list);
_speedup_dumps_cleanup_and_exit:
  Py_DECREF(empty);
  Py_DECREF(comma);
  Py_DECREF(arrow);
  Py_DECREF(s_null);
  Py_DECREF(citation);
  //Py_DECREF(list);
  //
  if (return_unicode == Py_True) {
      result = PyUnicode_FromEncodedObject(result, encoding, errors);
  }
  return result;
}


static PyMethodDef CPgHstoreMethods[] = {
    {"loads",  (PyCFunction)_speedups_loads, METH_VARARGS | METH_KEYWORDS,
     "Parse (decode) a postgres hstore string into a dict."},
    {"dumps",  (PyCFunction)_speedups_dumps, METH_VARARGS | METH_KEYWORDS,
     "Dump (encode) a dict into a postgres hstore string."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

PyMODINIT_FUNC
init_speedups(void)
{
    PyObject *m;

    m = Py_InitModule("pghstore._speedups", CPgHstoreMethods);
    if (m == NULL)
        return;
}

int
main(int argc, char *argv[])
{
    /* Pass argv[0] to the Python interpreter */
    Py_SetProgramName(argv[0]);

    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();

    /* Add a static module */
    init_speedups();

    return 0;
}
