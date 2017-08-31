"""Generate some hstore strings to use when testing pghstorers."""
import random
import string


def escape(s):
    """Escape backslashes and double-quotes."""
    return s.replace('\\', '\\\\').replace('"', '\\"')


population = list(string.ascii_lowercase + '0123456789"\\')
collection = {}
for i in range(100):
    key = escape(''.join(random.choices(population, k=128)))
    value = escape(''.join(random.choices(population, k=1024)))
    collection[key] = value


items = ['"{}" => "{}"'.format(*pair) for pair in collection.items()]
print(' , '.join(items))
