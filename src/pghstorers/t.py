"""Test module."""
import cffi

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

char *
hstore_dumps(struct HStoreItem *, size_t);

void
hstore_dumps_free(char *);

struct ParsedHStoreItem*
hstore_loads(char *, int *);

void
hstore_loads_free(struct ParsedHStoreItem *);
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


def loads(hstore_string):
    """Load a string into a dictionary."""
    hstore_char_ptr = char_from(hstore_string)
    returned_length = ffi.new("int *", 0)
    cdata = lib.hstore_loads(hstore_char_ptr, returned_length)
    cdata = ffi.gc(cdata, lib.hstore_loads_free)
    return_value = {}
    length = returned_length[0]
    print('Returned length of %d' % length)
    for i in range(length):
        item = cdata[i]
        import pdb; pdb.set_trace()
        print("i: %d, cdata.key: %r" % (i, item.key))
        value = None
        if item.value != ffi.NULL:
            value = ffi.string(item.value)
            print("Value: %s" % value)
        return_value[ffi.string(item.key)] = value
    return return_value
