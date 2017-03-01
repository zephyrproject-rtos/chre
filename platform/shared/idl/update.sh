#!/bin/bash
flatc --cpp -o ../include/chre/platform/shared/ --scoped-enums \
  --cpp-ptr-type chre::UniquePtr host_messages.fbs
