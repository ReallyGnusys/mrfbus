try:
    from io import BytesIO as StringIO
except ImportError:
    try:
        from io import StringIO
    except ImportError:
        from io import StringIO


__all__ = [
    'StringIO',
]
