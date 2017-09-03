#![feature(test)]
extern crate test;
extern crate pghstorers;

use std::io::Read;
use std::fs;
use test::Bencher;


#[bench]
fn benchmark_small_load_into_vec(b: &mut Bencher) {
    let contents = "\"key\" => \"value\", \"other_key\"  =>\"other_value\"";
    b.iter(|| { pghstorers::load::load_into_vec(contents); });
}


#[bench]
fn benchmark_medium_load_into_vec(b: &mut Bencher) {
    let mut test_data = fs::File::open("benches/medium_test_data").unwrap();
    let mut contents = String::new();
    let _read = test_data.read_to_string(&mut contents);
    b.iter(|| { pghstorers::load::load_into_vec(&contents as &str); });
}


#[bench]
fn benchmark_large_load_into_vec(b: &mut Bencher) {
    let mut test_data = fs::File::open("benches/large_test_data").unwrap();
    let mut contents = String::new();
    let _read = test_data.read_to_string(&mut contents);
    b.iter(|| { pghstorers::load::load_into_vec(&contents as &str); })
}


#[bench]
fn benchmark_small_load_into_hashmap(b: &mut Bencher) {
    let contents = "\"key\" => \"value\", \"other_key\"  =>\"other_value\"";
    b.iter(|| { pghstorers::load::load_into_hashmap(contents); });
}


#[bench]
fn benchmark_medium_load_into_hashmap(b: &mut Bencher) {
    let mut test_data = fs::File::open("benches/medium_test_data").unwrap();
    let mut contents = String::new();
    let _read = test_data.read_to_string(&mut contents);
    b.iter(|| {
        pghstorers::load::load_into_hashmap(&contents as &str);
    });
}


#[bench]
fn benchmark_large_load_into_hashmap(b: &mut Bencher) {
    let mut test_data = fs::File::open("benches/large_test_data").unwrap();
    let mut contents = String::new();
    let _read = test_data.read_to_string(&mut contents);
    b.iter(|| {
        pghstorers::load::load_into_hashmap(&contents as &str);
    })
}
