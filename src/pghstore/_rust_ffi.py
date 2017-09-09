"""Test module."""
import os
import sys

import cffi
import six

from pghstore import exceptions

ffi = cffi.FFI()
ffi.cdef("""
struct HStoreItem {
    char *key;
    char *value;
};

struct ParsedHStoreItem {
    char *key;
    char *value;
};

struct HStoreError {
    char *description;
};

char *
hstore_dumps(struct HStoreItem *, size_t);

void
hstore_dumps_free(char *);

struct ParsedHStoreItem*
hstore_loads(char *, int *, struct HStoreError *);

void
hstore_loads_free(struct ParsedHStoreItem *);
""")


library_type = 'dylib' if sys.platform == 'darwin' else 'so'
current_working_dir = os.path.abspath(os.path.dirname(__file__))
lib = ffi.dlopen(os.path.join(current_working_dir,
                              'libpghstorers.{}'.format(library_type)))


def char_from(s, encoding):
    """Create an FFI char* from the specified string.

    :param str s:
        String to create a new char * from.
    :returns:
        cffi.cdata
    """
    if isinstance(s, six.text_type):
        s = s.encode(encoding)
    return ffi.new('char[]', s)


def cdata_from(obj, encoding):
    """Create an FFI CData object."""
    if obj is None:
        return ffi.NULL
    return char_from(obj, encoding)


def error_ptr():
    """Create an HStoreError pointer."""
    err = ffi.new('struct HStoreError *')
    err.description = ffi.NULL
    return err


def dumps(dictionary, return_unicode=True, encoding='utf-8'):
    """Dump a dictionary to a string."""
    keypairs = [
        {
            "key": cdata_from(key, encoding),
            "value": cdata_from(dictionary[key], encoding),
        }
        for key in dictionary
    ]

    hstore_items = ffi.new("struct HStoreItem []", keypairs)
    cdata = lib.hstore_dumps(hstore_items, len(keypairs))
    cdata = ffi.gc(cdata, lib.hstore_dumps_free)
    string = ffi.string(cdata)
    if return_unicode and not isinstance(string, six.text_type):
        string = string.decode(encoding)
    return string


def _handle_cdata_items(items, encoding):
    for item in items:
        value = None
        key = ffi.string(item.key)
        key = key.decode(encoding)
        if item.value != ffi.NULL:
            value = ffi.string(item.value)
            value = value.decode(encoding)
        yield (key, value)


def _decode_cdata(cdata, encoding):
    if cdata == ffi.NULL:
        return None
    return ffi.string(cdata).decode('utf-8')


def loads(hstore_string, encoding='utf-8', return_type=dict):
    """Load a string into a dictionary."""
    error = error_ptr()
    hstore_char_ptr = char_from(hstore_string, encoding)
    returned_length = ffi.new("int *", 0)
    cdata = lib.hstore_loads(hstore_char_ptr, returned_length, error)

    length = returned_length[0]
    if cdata == ffi.NULL:
        if error.description != ffi.NULL:
            description = ffi.gc(error.description, lib.hstore_dumps_free)
            message = ffi.string(description)
            raise exceptions.ParseError(message, hstore_string)
        elif length <= 0:
            return return_type()

    cdata = ffi.gc(cdata, lib.hstore_loads_free)
    items = ffi.unpack(cdata, length)
    return return_type(
        (_decode_cdata(i.key, encoding), _decode_cdata(i.value, encoding))
        for i in items
    )
