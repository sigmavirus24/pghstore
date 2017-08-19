#include <Python.h>
#include <stdio.h>
#include <string.h>

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#endif

#define CHAR_CHECK(s, pos, c)                        \
  if (s[pos] != char) {                              \
    PyErr_Format(PyExc_ValueError,                   \
                 "Encountered %c at %i, was expecting %c.",   \
                 s[pos], pos, c);                             \
    return NULL;                                              \
  }

#define isEven(a) (((a) & 1) == 0)

#define ConcatOrGotoCleanup(result, string) { PyBytes_Concat(&result, string); if (!result) { goto _speedup_dumps_cleanup_and_exit; } }

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

/* Written to match pghstore._native.unescape but slightly smarter. */
char *
unescape(char *start_of_string, char *end_of_string)
{
  ssize_t copy_index = 0;
  ssize_t index;
  ssize_t string_length = (end_of_string - start_of_string) + 1;
  /* NOTE(sigmavirus24): In the event that there are \s in the string, we're
   * over-allocating here but that is okay since we're going to free shortly
   * after use.
   */
  char *unescaped_string = malloc(string_length);
  memset(unescaped_string, 0, string_length);

  for (index = 0; index < (string_length - 1); index++) {
      if (start_of_string[index] == '\\' && start_of_string[index + 1] != '\\') {
          continue;
      }
      unescaped_string[copy_index++] = start_of_string[index];
  }

  return unescaped_string;
}

static PyObject *
_speedups_loads(PyObject *self, PyObject *args, PyObject *keywds)
{
  static char *keyword_argument_names[] = {"string", "encoding", "return_type", NULL};
  const char *errors = "strict";
  char *encoding = "utf-8";
  char *s;
  int i, null_value = 0;
  char *unescaped_key, *unescaped_value;
  char *key_start, *key_end, *value_start, *value_end;
  PyTypeObject *return_type = &PyDict_Type;
  PyObject *return_value, *key, *value;

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|sO", keyword_argument_names, &s, &encoding, &return_type)) {
    return NULL;
  }

  return_value = PyObject_CallObject((PyObject *) return_type, NULL);
  
  // Each iteration will find one key/value pair
  while ((key_start = strchr(s, '"')) != NULL) {
    // move to the next character after the found "
    key_start++;

    // Find end of key
    key_end = strchr_unescaped(key_start, '"');

    // Find begining of value or NULL
    for (i=1; 1; i++) {
      switch (*(key_end+i)) {
      case 'N':
      case 'n':
        // found NULL
        null_value = 1;
        break;
      case '"':
        // found begining of value
        value_start = key_end+i+1;
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
     
    unescaped_key = unescape(key_start, key_end);
    key = PyUnicode_Decode(unescaped_key, strlen(unescaped_key), encoding, errors);
    if (null_value == 0) {
      // find and null terminate end of value
      value_end = strchr_unescaped(value_start, '"');
      unescaped_value = unescape(value_start, value_end);
      value = PyUnicode_Decode(unescaped_value, strlen(unescaped_value), encoding, errors);
      free(unescaped_value);
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
      s = value_end + 1;
    } else {
      s = key_end + i;
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
  PyObject *obj, *list, *iter, *item;
  PyObject *return_unicode = Py_False;
  PyObject *key_map_callback = Py_None;
  Py_INCREF(Py_None);
  PyObject *value_map_callback = Py_None;
  Py_INCREF(Py_None);
  PyObject *unencoded_key, *key, *unencoded_value, *value, *result;
  PyObject *comma, *arrow, *empty, *s_null, *citation;
  PyObject *exception_string = NULL;
  PyObject *exception_string_format_args = NULL;
  Py_ssize_t list_len = 0;

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOsO", keyword_argument_names, &obj, &key_map_callback, &value_map_callback, &encoding, &return_unicode)) {
    return NULL;
  }

  if (PyCallable_Check(key_map_callback)) {
      Py_INCREF(key_map_callback);
  }

  if (PyCallable_Check(value_map_callback)) {
      Py_INCREF(value_map_callback);
  }

  // return empty string if we got an empty dict
  empty = PyBytes_FromString(EMPTY);

  if (PyDict_Check(obj)) {
      list = PyDict_Items(obj);
  } else {
      list = obj;
      Py_INCREF(obj);
  }

  list_len = PyObject_Length(list);
  if (list_len <= 0) {
    return empty;
  }
  
  // create string constants
  comma = PyBytes_FromString(COMMA);
  arrow = PyBytes_FromString(ARROW);
  s_null = PyBytes_FromString(S_NULL);
  citation = PyBytes_FromString(CITATION);
  
  result = PyBytes_FromString(EMPTY);
  iter = PyObject_GetIter(list);

  while ((item = PyIter_Next(iter))) {
    // add comma (,)
    if (i > 0) {
      ConcatOrGotoCleanup(result, comma);
    }
    // add key
    ConcatOrGotoCleanup(result, citation);
    unencoded_key = PyTuple_GetItem(item, 0);

    if (key_map_callback != Py_None && PyCallable_Check(key_map_callback)) {
        unencoded_key = PyObject_CallObject(key_map_callback, PyTuple_Pack(1, unencoded_key));
    }

    if (PyUnicode_Check(unencoded_key)) {
        key = PyUnicode_AsEncodedString(unencoded_key, encoding, errors);
    } else {
        if (PyBytes_Check(unencoded_key)) {
            key = unencoded_key;
            Py_INCREF(unencoded_key);
        } else {
            key = PyObject_Bytes(unencoded_key);
        }
    }

    unencoded_value = PyTuple_GetItem(item, 1);
    if (value_map_callback != Py_None && PyCallable_Check(value_map_callback)) {
        unencoded_value = PyObject_CallObject(value_map_callback, PyTuple_Pack(1, unencoded_value));
    }
    if (PyUnicode_Check(unencoded_value)) {
        value = PyUnicode_AsEncodedString(unencoded_value, encoding, errors);
    } else {
        value = unencoded_value;
        Py_INCREF(unencoded_value);
    }

    ConcatOrGotoCleanup(result, key);
    ConcatOrGotoCleanup(result, citation);
    // add arrow (=>)
    ConcatOrGotoCleanup(result, arrow);
    // add value or null
    if (value != Py_None) {
      if (!PyBytes_Check(value)) {
          exception_string = PyUnicode_FromString("value %r of %r is not a string");
          exception_string_format_args = PyTuple_Pack(2, value, key);
          PyErr_SetObject(PyExc_TypeError, PyUnicode_Format(exception_string, exception_string_format_args));
          Py_CLEAR(result);
          goto _speedup_dumps_cleanup_and_exit;
      }
      // add value
      ConcatOrGotoCleanup(result, citation);
      ConcatOrGotoCleanup(result, PyObject_Bytes(value));
      ConcatOrGotoCleanup(result, citation);
    } else {
      // add null
      ConcatOrGotoCleanup(result, empty);
      ConcatOrGotoCleanup(result, s_null);
      ConcatOrGotoCleanup(result, empty);

    }
    Py_DECREF(item);
    Py_DECREF(key);
    Py_DECREF(value);
    i++;
  }
_speedup_dumps_cleanup_and_exit:
  Py_DECREF(empty);
  Py_DECREF(comma);
  Py_DECREF(arrow);
  Py_DECREF(s_null);
  Py_DECREF(citation);
  Py_DECREF(iter);
  Py_DECREF(list);
  Py_XDECREF(value_map_callback);
  Py_XDECREF(key_map_callback);

  if (return_unicode == Py_True) {
      result = PyUnicode_FromEncodedObject(result, encoding, errors);
  }
  return result;
}


static PyMethodDef CPgHstoreMethods[] = {
    {"loads",  (PyCFunction)_speedups_loads, METH_VARARGS | METH_KEYWORDS,
     "Parse (decode) a postgres hstore string into an object."},
    {"dumps",  (PyCFunction)_speedups_dumps, METH_VARARGS | METH_KEYWORDS,
     "Dump (encode) a object into a postgres hstore string."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

#if PY_MAJOR_VERSION >= 3
static int pghstore_speedups_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int pghstore_speedups_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "pghstore._speedups",
    NULL,
    sizeof(struct module_state),
    CPgHstoreMethods,
    NULL,
    pghstore_speedups_traverse,
    pghstore_speedups_clear,
    NULL
};

PyMODINIT_FUNC
PyInit__speedups(void)
{
    PyObject *module;

    module = PyModule_Create(&moduledef);
    if (module == NULL) {
        return NULL;
    }

    return module;
}
#else
PyMODINIT_FUNC
init_speedups(void)
{
    (void) Py_InitModule("pghstore._speedups", CPgHstoreMethods);
}
#endif

int
main(int argc, char *argv[])
{
#if PY_MAJOR_VERSION >= 3
    wchar_t *program = Py_DecodeLocale(argv[0], NULL);
    /* Pass program name to the Python interpreter */
    Py_SetProgramName(program);
#else
    Py_SetProgramName(argv[0]);
#endif

    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();

#if PY_MAJOR_VERSION < 3
    /* Add a static module */
    init_speedups();
#endif

    return 0;
}