use std::collections::HashMap;
use std::string::String;


fn escape(to_escape: &String) -> String {
    to_escape
        .replace("\\", "\\\\")
        .replace("\"", "\\\"")
}

pub fn dump_keypair(key: &String, value: &Option<String>) -> String {
    let escaped_key = escape(key);
    let escaped_value = match *value {
        Some(ref unescaped_value) => format!("\"{}\"", escape(unescaped_value)),
        None => "NULL".to_string(),
    };
    format!("\"{}\"=>{}", escaped_key, escaped_value)
}


pub fn dump_hashmap(map: &HashMap<String, Option<String>>) -> String {
    let pairs = map.iter().map(|(key, value)| dump_keypair(key, value));
    pairs.collect::<Vec<_>>().join(",")
}


#[cfg(test)]
mod tests {
    use super::{dump_keypair, dump_hashmap};
    use std::collections::HashMap;

    #[test]
    fn dump_keypair_converts_to_hstore_key_value() {
        assert_eq!(
            dump_keypair(&"my_key".to_string(), &Some("my_value".to_string())),
            "\"my_key\"=>\"my_value\""
        );
    }

    #[test]
    fn dump_keypair_handles_null_value() {
        assert_eq!(
            dump_keypair(&"my_key".to_string(), &None),
            "\"my_key\"=>NULL"
        );
    }

    #[test]
    fn converts_to_hstore_key_value_with_quotes() {
        assert_eq!(
            dump_keypair(
                &"my_\"quoted\"_key".to_string(),
                &Some("my_\"quoted\"_value".to_string()),
            ),
            "\"my_\\\"quoted\\\"_key\"=>\"my_\\\"quoted\\\"_value\""
        );
    }

    #[test]
    fn converts_to_hstore_key_value_with_backslashes() {
        assert_eq!(
            dump_keypair(
                &"my_\\escaped\\_key".to_string(),
                &Some("my_\\escaped\\_value".to_string()),
            ),
            "\"my_\\\\escaped\\\\_key\"=>\"my_\\\\escaped\\\\_value\""
        );
    }

    #[test]
    fn converts_hashmap_to_string() {
        let mut map: HashMap<String, Option<String>> = HashMap::new();
        map.insert("null_key".to_string(), None);
        map.insert("a_key".to_string(), Some("a".to_string()));
        let dumped = dump_hashmap(&map);
        assert!(dumped.contains("\"null_key\"=>NULL"));
        assert!(dumped.contains("\"a_key\"=>\"a\""));
    }
}
