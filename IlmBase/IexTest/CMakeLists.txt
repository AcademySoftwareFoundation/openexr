# yue.nicholas@gmail.com

ADD_EXECUTABLE ( IexTest
  main.cpp
  testBaseExc.cpp
  )

TARGET_LINK_LIBRARIES ( IexTest IlmBase::Iex${OPENEXR_TARGET_SUFFIX} )

ADD_TEST ( TestIex IexTest )
