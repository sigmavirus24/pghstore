"""Test module."""
import cffi

ffi = cffi.FFI()
ffi.cdef("""
struct HStoreItem {
    char *key;
    char *value;
};

char *hstore_dumps(struct HStoreItem *, size_t);
char *hstore_dumps_keypair(char *, char *);
void  hstore_dumps_free(char *);
""")


lib = ffi.dlopen('./target/debug/libpghstorers.so')


def char_from(s):
    """Create an FFI char* from the specified string.

    :param str s:
        String to create a new char * from.
    :returns:
        cffi.cdata
    """
    return ffi.new('char[]', s)


def cdata_from(obj):
    """Create an FFI CData object."""
    if obj is None:
        return ffi.NULL
    return char_from(obj)


def dumps(dictionary):
    """Dump a dictionary to a string."""
    keypairs = [
        {
            "key": cdata_from(key),
            "value": cdata_from(dictionary[key]),
        }
        for key in dictionary
    ]

    hstore_items = ffi.new("struct HStoreItem []", keypairs)
    cdata = lib.hstore_dumps(hstore_items, len(keypairs))
    cdata = ffi.gc(cdata, lib.hstore_dumps_free)
    return ffi.string(cdata)
