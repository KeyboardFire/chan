#include "net.h"

#include <stdlib.h>
#include <string.h>

struct membuf {
    char *data;
    size_t len;
};

size_t nop_callback(char *data, size_t size, size_t nmemb, void *userp) {
    return size * nmemb;
}

size_t mem_callback(char *data, size_t size, size_t nmemb, void *userp) {
    size_t len = size * nmemb;
    struct membuf *buf = (struct membuf *) userp;
    buf->data = realloc(buf->data, buf->len + len);
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
    return len;
}

char *http(CURL *curl, const char *url, const char *postfields, int data) {
    curl_easy_setopt(curl, postfields ? CURLOPT_POST : CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url);

    if (postfields) {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);
    }

    struct membuf buf;
    if (data) {
        buf.data = NULL;
        buf.len = 0;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nop_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL);
    }

    curl_easy_perform(curl);

    return data ? buf.data : NULL;
}
