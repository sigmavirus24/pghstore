"""Exceptions module."""


class HStoreError(Exception):
    """Base class for all pghstore exceptions."""

    pass


class ParseError(HStoreError):
    """Class raised when there are exceptions encountered during parsing."""

    pass
