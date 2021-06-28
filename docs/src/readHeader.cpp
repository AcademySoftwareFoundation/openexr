void
readHeader (const char fileName[])
{
    RgbaInputFile file (fileName);
        
    const StringAttribute *comments =
        file.header().findTypedAttribute <StringAttribute> ("comments");
        
    const M44fAttribute *cameraTransform =
        file.header().findTypedAttribute <M44fAttribute> ("cameraTransform");
        
    if (comments)
        cout << "commentsn " << comments->value() << endl;
    
    if (cameraTransform)
        cout << "cameraTransformn" << cameraTransform->value() << flush;
}
