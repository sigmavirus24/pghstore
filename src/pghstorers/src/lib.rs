extern crate libc;

use libc::{c_char, size_t};
use std::collections::HashMap;
use std::ffi::{CStr, CString};
use std::slice;
use std::string::String;

pub mod dump;


#[repr(C)]
pub struct HStoreItem {
    key: *const c_char,
    value: *const c_char,
}

fn convert_cchar(value: *const c_char) -> String {
    unsafe {
        assert!(!value.is_null());
        CStr::from_ptr(value)
    }.to_str()
        .unwrap()
        .to_string()
}

#[no_mangle]
pub extern "C" fn hstore_dumps(items: *const HStoreItem, len: size_t) -> *mut c_char {
    let slice = unsafe {
        assert!(!items.is_null());
        slice::from_raw_parts(items, len as usize)
    };
    let map = slice.iter().fold(HashMap::new(), |mut map, ref item| {
        let key = convert_cchar(item.key);
        let value = match item.value.is_null() {
            true => None,
            false => Some(convert_cchar(item.value)),
        };
        map.insert(key, value);
        map
    });

    let dump = dump::dump_hashmap(&map);

    CString::new(dump).unwrap().into_raw()
}

#[no_mangle]
pub extern "C" fn hstore_dumps_free(string: *mut c_char) {
    unsafe {
        if string.is_null() {
            return;
        }
        CString::from_raw(string)
    };
}
