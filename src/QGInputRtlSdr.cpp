#include "QGInputRtlSdr.h"

#include <iostream>
#include <stdexcept>

QGInputRtlSdr::QGInputRtlSdr(const YAML::Node &config): QGInputDevice(config) {
}

QGInputRtlSdr::~QGInputRtlSdr() {
}

void QGInputRtlSdr::open() {
}
