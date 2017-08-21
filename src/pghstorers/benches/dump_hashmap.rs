#![feature(test)]
extern crate test;
extern crate pghstorers;

use std::collections::HashMap;

use test::Bencher;


#[bench]
fn benchmark_dump_hashmap(b: &mut Bencher) {
    let alphabet = [
        "a",
        "b",
        "c",
        "d",
        "e",
        "f",
        "g",
        "h",
        "i",
        "j",
        "k",
        "l",
        "m",
        "n",
        "o",
        "p",
        "q",
        "r",
        "s",
        "t",
        "u",
        "v",
        "w",
        "x",
        "y",
        "z",
    ];
    let mut map: HashMap<String, Option<String>> = HashMap::new();
    for i in 0..26 {
        for character in alphabet.iter() {
            let key = format!("{}{}", character, i);
            let value = Some(character.to_string());
            map.insert(key, value);
        }
    }
    b.iter(|| {
        pghstorers::dump::dump_hashmap(&map);
    })
}
