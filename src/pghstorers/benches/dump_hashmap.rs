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
            let longer_key = format!("very_very_very_very_very_very_long_{}_{}", character, i);
            let longer_value = Some(
                format!("uber_uber_uber_very_very_very_very_very_very_long_{}_{}", character, i)
            );

            map.insert(key, value);
            map.insert(longer_key, longer_value);
        }
    }
    b.iter(|| {
        pghstorers::dump::dump_hashmap(&map);
    })
}
