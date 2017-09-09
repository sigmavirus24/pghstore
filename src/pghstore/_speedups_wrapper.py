from pghstore import _speedups
from pghstore import exceptions


def loads(string, encoding='utf-8', return_type=dict):
    try:
        return _speedups.loads(string, encoding=encoding,
                               return_type=return_type)
    except UnicodeDecodeError as err:
        raise exceptions.ParseError(*err.args)


def dumps(obj, key_map=None, value_map=None, encoding='utf-8',
          return_unicode=False):
    return _speedups.dumps(obj, key_map=key_map, value_map=value_map,
                           encoding=encoding, return_unicode=return_unicode)
