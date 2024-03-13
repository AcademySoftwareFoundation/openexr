
int width=1; int height=1;
// [begin setCompression]
Header header (width, height);
header.channels().insert ("G", Channel (HALF));
header.channels().insert ("Z", Channel (FLOAT));
header.compression() = ZIP_COMPRESSION;
header.zipCompressionLevel() = 6;
// [end setCompression]

// [begin setCompressionDefault]
setDefaultZipCompressionLevel (6);
setDefaultDwaCompressionLevel (45.0f);
// [end setCompressionDefault]