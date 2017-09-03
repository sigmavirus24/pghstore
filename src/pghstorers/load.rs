use std::cell::Cell;
use std::collections::HashMap;
use std::iter::{Iterator, Peekable};
use std::str::Chars;
use std::string::String;

pub fn load_into_vec<S: Into<String>>(hstore_string: S) -> Vec<(String, Option<String>)> {
    let to_parse = hstore_string.into();
    let parser = HStoreParser::from(&to_parse);
    parser.collect::<Vec<(String, Option<String>)>>()
}


pub fn load_into_hashmap<S: Into<String>>(hstore_string: S) -> HashMap<String, Option<String>> {
    let to_parse = hstore_string.into();
    let parser = HStoreParser::from(&to_parse);
    parser.collect::<HashMap<String, Option<String>>>()
}


fn collect_and_unescape<'a>(chars: &mut Peekable<Chars<'a>>, value: &mut String) {
    loop {
        let next = chars.next();
        let next_char = match next {
            Some('\\') => {
                match chars.peek() {
                    Some(&'"') | Some(&'\\') => chars.next(),
                    _ => None,
                }
            }
            Some('"') | None => break,
            _ => next,
        };

        if let Some(c) = next_char {
            value.push(c);
        }
    }
}


fn skip_chars_while<'a>(chars: &mut Peekable<Chars<'a>>, predicate: fn(Option<&char>) -> bool) {
    loop {
        if predicate(chars.peek()) {
            chars.next();
        } else {
            break;
        }
    }
}


fn is_null<'a>(chars: &mut Peekable<Chars<'a>>) -> bool {
    let mut null_strings: Vec<String> = Vec::new();
    loop {
        if let Some(c) = chars.next() {
            if c == ',' {
                break;
            }
            null_strings.push(c.to_lowercase().to_string())
        } else {
            break;
        }
    }
    let value = null_strings.join("");
    value == "null"
}


pub struct HStoreParser<'a> {
    chars: Cell<Peekable<Chars<'a>>>,
}


impl<'a> HStoreParser<'a> {
    pub fn from(hstore_string: &String) -> HStoreParser {
        let chars = hstore_string.chars().peekable();
        HStoreParser { chars: Cell::new(chars) }
    }
}


impl<'a> Iterator for HStoreParser<'a> {
    type Item = (String, Option<String>);

    fn next(&mut self) -> Option<Self::Item> {
        let mut chars = self.chars.get_mut();
        if chars.peek() == None {
            return None;
        }
        skip_chars_while(&mut chars, |c| match c {
            // We want to stop if we hit None
            Some(&'"') | None => false,
            _ => true,
        });
        // Skip the dquote if it exists
        if chars.next() == None {
            return None;
        }
        let mut key = String::new();
        let mut value = String::new();
        collect_and_unescape(&mut chars, &mut key);
        skip_chars_while(&mut chars, |c| match c {
            Some(&' ') | Some(&'=') | Some(&'>') => true,
            _ => false,
        });
        match chars.peek() {
            Some(&'"') => {
                chars.next(); // skip the dquote
                collect_and_unescape(&mut chars, &mut value);
                Some((key, Some(value)))
            }
            Some(&'N') | Some(&'n') => {
                if is_null(&mut chars) {
                    Some((key, None))
                } else {
                    panic!("Error parsing value for key ```{}```", key);
                }
            }
            _ => None,
        }
    }
}


#[cfg(test)]
mod tests {
    use super::{load_into_vec, load_into_hashmap};

    #[test]
    fn handles_empty_string() {
        let s: &str = "";
        let pairs_vec = load_into_vec(s);
        assert_eq!(pairs_vec.len(), 0);
    }

    #[test]
    fn parses_a_single_pair() {
        let s: &str = "\"key\"=>\"value\"";
        let pairs_vec = load_into_vec(s);
        assert_eq!(pairs_vec.len(), 1);
        let pairs_map = load_into_hashmap(s);
        assert_eq!(pairs_map.len(), 1);
    }

    #[test]
    fn allows_spaces_around_hashrocket() {
        let s = String::from("\"key\"  =>  \"value\"");
        let pairs_map = load_into_hashmap(s);
        assert_eq!(pairs_map.len(), 1);
    }

    #[test]
    fn unescapes_escaped_quotes() {
        let s = String::from(
            "\"key_\\\"with\\\"_quote\"  =>  \"value_\\\"with\\\"_quote\"",
        );
        let value = String::from("value_\"with\"_quote");
        let pairs_map = load_into_hashmap(s);
        assert_eq!(pairs_map.len(), 1);
        assert_eq!(pairs_map.get("key_\"with\"_quote"), Some(&Some(value)));
    }

    #[test]
    fn unescapes_escaped_escape_characters() {
        let s = String::from(
            "\"key_\\\\with\\\\_backslash\"  =>  \"value_\\\\with\\\\_backslash\"",
        );
        let value = String::from("value_\\with\\_backslash");
        let pairs_map = load_into_hashmap(s);
        assert_eq!(pairs_map.len(), 1);
        assert_eq!(pairs_map.get("key_\\with\\_backslash"), Some(&Some(value)));
    }

    #[test]
    fn handles_null() {
        let s: &str = "\"key\"=>NULL";
        let map = load_into_hashmap(s);
        assert_eq!(map.get("key"), Some(&None));
    }

    #[test]
    fn handles_null_and_others() {
        let s: &str = "\"key\"=>NULL,\"a_following_key\"=>\"value\"";
        let map = load_into_hashmap(s);
        assert_eq!(map.len(), 2);
    }
}
