# yue.nicholas@gmail.com

ADD_EXECUTABLE ( IexTest
  main.cpp
  testBaseExc.cpp
  )

TARGET_LINK_LIBRARIES ( IexTest Iex )

INSTALL ( TARGETS
  IexTest
    RUNTIME DESTINATION bin
)

ADD_TEST ( TestIex IexTest )
