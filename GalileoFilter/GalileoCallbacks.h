#pragma once

typedef struct _FLT_FILTER *PFLT_FILTER;
typedef struct _FLT_CALLBACK_DATA *PFLT_CALLBACK_DATA;
typedef const struct _FLT_RELATED_OBJECTS *PCFLT_RELATED_OBJECTS;

#ifdef __cplusplus
extern "C" {
#endif

void Initialize(PFLT_FILTER filter);
void UnInitialize();

int PreOperation(PFLT_CALLBACK_DATA data, PCFLT_RELATED_OBJECTS objects);
void PostOperation(PFLT_CALLBACK_DATA data);

#ifdef __cplusplus
}
#endif