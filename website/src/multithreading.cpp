char fileName[] = "";
// [begin main thread create]
// main, before application threads are created:
    
setGlobalThreadCount (4);
// [begin applications input thread]
// application's input thread

InputFile in (fileName);

// ...
// [end applications input thread]

Header header = in.header();
// [begin applications output thread]
// application's output thread

OutputFile out (fileName, header, 2);

// ...
// [end applications output thread]

