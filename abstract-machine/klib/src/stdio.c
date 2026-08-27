#include <klib.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// ---- Minimal vsnprintf engine: everything else is built on top of it. ----
// Supports: %d (int), %u (unsigned int), %x/%X (hex), %s (string),
// %c (char), %% (literal percent). No width/precision/padding support --
// enough to cover typical test/benchmark usage.

static void __out_char(char *buf, size_t size, size_t *pos, char c) {
  if (*pos < size) buf[*pos] = c;
  (*pos)++;
}

static void __out_str(char *buf, size_t size, size_t *pos, const char *s) {
  while (*s) __out_char(buf, size, pos, *s++);
}

static void __out_uint(char *buf, size_t size, size_t *pos, unsigned long long val, int base, int upper) {
  char tmp[32];
  int i = 0;
  const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
  if (val == 0) {
    tmp[i++] = '0';
  } else {
    while (val > 0) {
      tmp[i++] = digits[val % base];
      val /= base;
    }
  }
  while (i > 0) __out_char(buf, size, pos, tmp[--i]);
}

static void __out_int(char *buf, size_t size, size_t *pos, long long val) {
  if (val < 0) {
    __out_char(buf, size, pos, '-');
    val = -val;
  }
  __out_uint(buf, size, pos, (unsigned long long)val, 10, 0);
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  size_t pos = 0;
  const char *p = fmt;

  while (*p) {
    if (*p != '%') {
      __out_char(out, n, &pos, *p++);
      continue;
    }
    p++; // skip '%'
    switch (*p) {
      case 'd': {
        int v = va_arg(ap, int);
        __out_int(out, n, &pos, v);
        break;
      }
      case 'u': {
        unsigned int v = va_arg(ap, unsigned int);
        __out_uint(out, n, &pos, v, 10, 0);
        break;
      }
      case 'x': {
        unsigned int v = va_arg(ap, unsigned int);
        __out_uint(out, n, &pos, v, 16, 0);
        break;
      }
      case 'X': {
        unsigned int v = va_arg(ap, unsigned int);
        __out_uint(out, n, &pos, v, 16, 1);
        break;
      }
      case 's': {
        const char *s = va_arg(ap, const char *);
        if (!s) s = "(null)";
        __out_str(out, n, &pos, s);
        break;
      }
      case 'c': {
        char c = (char)va_arg(ap, int);
        __out_char(out, n, &pos, c);
        break;
      }
      case '%': {
        __out_char(out, n, &pos, '%');
        break;
      }
      default: {
        __out_char(out, n, &pos, '%');
        __out_char(out, n, &pos, *p);
        break;
      }
    }
    p++;
  }

  if (n > 0) {
    if (pos < n) out[pos] = '\0';
    else out[n - 1] = '\0';
  }

  return (int)pos;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, (size_t)-1, fmt, ap);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return r;
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsprintf(out, fmt, ap);
  va_end(ap);
  return r;
}

int vprintf(const char *fmt, va_list ap) {
  char buf[256];
  int r = vsnprintf(buf, sizeof(buf), fmt, ap);
  for (int i = 0; i < r && i < (int)sizeof(buf) - 1; i++) putch(buf[i]);
  return r;
}

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vprintf(fmt, ap);
  va_end(ap);
  return r;
}

int __am_vsscanf_internal(const char *str, const char **end_pstr, const char *fmt, va_list ap) {
  const char *pstr = str;
  const char *pfmt = fmt;
  int item = -1;
  while (*pfmt) {
    char ch = *pfmt ++;
    if (isspace(ch)) {
      for (ch = *pfmt; isspace(ch); ch = *(++ pfmt));
      for (ch = *pstr; isspace(ch); ch = *(++ pstr));
      item ++;
      continue;
    }
    switch (ch) {
      case '%': break;
      default:
        if (*pstr == ch) { // match
          pstr ++;
          item ++;
          continue;
        }
        goto end; // fail
    }

    char *p;
    ch = *pfmt ++;
    switch (ch) {
      // conversion specifier
      case 'd':
        *(va_arg(ap, int *)) = strtol(pstr, &p, 10);
        if (p == pstr) goto end; // fail
        pstr = p;
        item ++;
        break;

      case 'c':
        *(va_arg(ap, char *)) = *pstr ++;
        item ++;
        break;

      default:
        printf("Unsupported conversion specifier '%c'\n", ch);
        assert(0);
    }
  }

end:
  if (end_pstr) {
    *end_pstr = pstr;
  }
  return item;
}

int vsscanf(const char *str, const char *fmt, va_list ap) {
  return __am_vsscanf_internal(str, NULL, fmt, ap);
}

int sscanf(const char *str, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsscanf(str, fmt, ap);
  va_end(ap);
  return r;
}

int __isoc99_sscanf(const char *str, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vsscanf(str, fmt, ap);
  va_end(ap);
  return r;
}

#endif
