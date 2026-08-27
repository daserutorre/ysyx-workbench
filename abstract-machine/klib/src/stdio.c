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

    // Parse an optional minimum field width, e.g. "10" in "%10u".
    int width = 0;
    while (*p >= '0' && *p <= '9') {
      width = width * 10 + (*p - '0');
      p++;
    }

    // Parse an optional length modifier: "l" or "ll" (we only distinguish
    // "64-bit" vs "not", which covers both since we always widen anyway).
    int is_long = 0;
    while (*p == 'l') {
      is_long = 1;
      p++;
    }

    // Render the converted value into a small local buffer first, so we
    // can measure its length and apply width-padding before copying it
    // into the real output.
    char conv[32];
    size_t conv_len = 0;

    switch (*p) {
      case 'd': {
        long long v = is_long ? va_arg(ap, long long) : (long long)va_arg(ap, int);
        __out_int(conv, sizeof(conv), &conv_len, v);
        break;
      }
      case 'u': {
        unsigned long long v = is_long ? va_arg(ap, unsigned long long) : (unsigned long long)va_arg(ap, unsigned int);
        __out_uint(conv, sizeof(conv), &conv_len, v, 10, 0);
        break;
      }
      case 'x': {
        unsigned long long v = is_long ? va_arg(ap, unsigned long long) : (unsigned long long)va_arg(ap, unsigned int);
        __out_uint(conv, sizeof(conv), &conv_len, v, 16, 0);
        break;
      }
      case 'X': {
        unsigned long long v = is_long ? va_arg(ap, unsigned long long) : (unsigned long long)va_arg(ap, unsigned int);
        __out_uint(conv, sizeof(conv), &conv_len, v, 16, 1);
        break;
      }
      case 's': {
        const char *s = va_arg(ap, const char *);
        if (!s) s = "(null)";
        __out_str(conv, sizeof(conv), &conv_len, s);
        break;
      }
      case 'c': {
        char c = (char)va_arg(ap, int);
        __out_char(conv, sizeof(conv), &conv_len, c);
        break;
      }
      case '%': {
        __out_char(conv, sizeof(conv), &conv_len, '%');
        break;
      }
      default: {
        __out_char(conv, sizeof(conv), &conv_len, '%');
        __out_char(conv, sizeof(conv), &conv_len, *p);
        break;
      }
    }

    // Right-align with spaces if the converted value is shorter than the
    // requested minimum width.
    for (size_t i = conv_len; i < (size_t)width; i++) {
      __out_char(out, n, &pos, ' ');
    }
    for (size_t i = 0; i < conv_len; i++) {
      __out_char(out, n, &pos, conv[i]);
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
