extern crate libc;

use libc::{c_char, size_t};
use std::collections::HashMap;
use std::error::Error;
use std::ffi::{CStr, CString};
use std::mem;
use std::ptr::null_mut;
use std::slice;
use std::str;
use std::string::String;

pub mod dump;
pub mod load;


#[repr(C)]
pub struct HStoreItem {
    key: *const c_char,
    value: *const c_char,
}

#[repr(C)]
pub struct ParsedHStoreItem {
    key: *mut c_char,
    value: *mut c_char,
}

#[repr(C)]
pub struct HStoreError {
    description: *mut c_char,
}

fn convert_cchar(value: *const c_char) -> Result<String, str::Utf8Error> {
    let c_str = unsafe {
        assert!(!value.is_null());
        CStr::from_ptr(value)
    }.to_str();
    match c_str {
        Ok(s) => Ok(s.to_string()),
        Err(err) => Err(err),
    }
}

#[no_mangle]
pub extern "C" fn hstore_dumps(items: *const HStoreItem, len: size_t) -> *mut c_char {
    let slice = unsafe {
        assert!(!items.is_null());
        slice::from_raw_parts(items, len as usize)
    };
    let map = slice.iter().fold(HashMap::new(), |mut map, ref item| {
        let key = convert_cchar(item.key).unwrap();
        let value = match item.value.is_null() {
            true => None,
            false => Some(convert_cchar(item.value).unwrap()),
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
        CString::from_raw(string);
    }
}

fn pair_to_item(pair: &(String, Option<String>)) -> ParsedHStoreItem {
    let c_key = CString::new(pair.0.to_string()).unwrap().into_raw();
    let c_value: *mut c_char = match pair.1 {
        Some(ref value) => CString::new(value.to_string()).unwrap().into_raw(),
        None => null_mut(),
    };
    ParsedHStoreItem {
        key: c_key,
        value: c_value,
    }
}

#[no_mangle]
pub extern "C" fn hstore_loads(
    string: *const c_char,
    length: *mut size_t,
    error: *mut HStoreError,
) -> *mut ParsedHStoreItem {
    let hstore_string_result = convert_cchar(string);
    if let Err(err) = hstore_string_result {
        unsafe {
            (*error).description = CString::new(err.description().to_string())
                .unwrap()
                .into_raw();
        }
        return null_mut();
    }
    let hstore_string = hstore_string_result.unwrap();

    let parser = load::HStoreParser::from(&hstore_string);
    let items_vec: Vec<ParsedHStoreItem> = parser
        .map(|pair| pair_to_item(&pair))
        .collect();
    if items_vec.len() <= 0 {
        unsafe { *length = 0; }
        return null_mut();
    }
    // let string_pairs = load::load_into_vec(hstore_string);
    // if string_pairs.len() <= 0 {
    //     unsafe { *length = 0; }
    //     return null_mut();
    // }
    // let items_vec: Vec<ParsedHStoreItem> = string_pairs
    //     .iter()
    //     .map(|pair| pair_to_item(pair))
    //     .collect();
    let mut items_slice: Box<[ParsedHStoreItem]> = items_vec.into_boxed_slice();
    unsafe { *length = items_slice.len(); }
    let ptr: *mut ParsedHStoreItem = items_slice.as_mut_ptr();
    mem::forget(items_slice);
    ptr
}

#[no_mangle]
pub extern "C" fn hstore_loads_free(items: *mut ParsedHStoreItem) {
    if items.is_null() {
        return;
    }
    unsafe {
        Box::from_raw(items);
    }
}
