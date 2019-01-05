typedef struct tagBMPRESOURCE {
    int name;
    MWIMAGEHDR *bmp;
} BMPRESOURCE;


void WINAPI SetBitmapResources ( BMPRESOURCE *res );

