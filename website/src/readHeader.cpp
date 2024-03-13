// [begin readHeader]
void
readHeader (const char fileName[])
{
    RgbaInputFile file (fileName);

    const StringAttribute* comments =
        file.header ().findTypedAttribute<StringAttribute> ("comments");

    const M44fAttribute* cameraTransform =
        file.header ().findTypedAttribute<M44fAttribute> ("cameraTransform");

    if (comments) cout << "commentsn " << comments->value () << endl;

    if (cameraTransform)
        cout << "cameraTransformn" << cameraTransform->value () << flush;
}
// [end readHeader]
 
// [begin readComments]
void
readComments (const char fileName[], string &comments)
{
    RgbaInputFile file (fileName);

    comments = file.header().typedAttribute<StringAttribute>("comments").value();
}
// [end readComments]

// [begin readCommentsError]
void
readComments (const char fileName[], const StringAttribute *&comments)
{
    // error: comments pointer is invalid after this function returns

    RgbaInputFile file (fileName);

    comments = file.header().findTypedAttribute <StringAttribute> ("comments");
}
// [end readCommentsError]
