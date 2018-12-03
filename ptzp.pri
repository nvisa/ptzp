HEADERS += \
    ptzp/ioerrors.h \
    ptzp/irdomepthead.h \
    ptzp/mgeothermalhead.h \
    ptzp/oemmodulehead.h \
    ptzp/ptzpdriver.h \
    ptzp/ptzphead.h \
    ptzp/ptzpserialtransport.h \
    ptzp/ptzptcptransport.h \
    ptzp/ptzptransport.h \
    ptzp/irdomedriver.h \
    ptzp/aryadriver.h \
    ptzp/aryapthead.h \
    ptzp/gungorhead.h \
    ptzp/yamanolenshead.h \
    ptzp/tbgthdriver.h

SOURCES += \
    ptzp/irdomepthead.cpp \
    ptzp/mgeothermalhead.cpp \
    ptzp/oemmodulehead.cpp \
    ptzp/ptzpdriver.cpp \
    ptzp/ptzphead.cpp \
    ptzp/ptzpserialtransport.cpp \
    ptzp/ptzptcptransport.cpp \
    ptzp/ptzptransport.cpp \
    ptzp/irdomedriver.cpp \
    ptzp/aryadriver.cpp \
    ptzp/aryapthead.cpp \
    ptzp/gungorhead.cpp \
    ptzp/yamanolenshead.cpp \
    ptzp/tbgthdriver.cpp

ptzp-grpc {
    DEFINES += HAVE_PTZP_GRPC_API

    LIBS += -L/usr/local/lib -lprotobuf -lgrpc++

    QMAKE_EXTRA_VARIABLES = GRPC_CPP_PLUGIN GRPC_CPP_PLUGIN_PATH
    GRPC_CPP_PLUGIN = grpc_cpp_plugin
    GRPC_CPP_PLUGIN_PATH = `which $(EXPORT_GRPC_CPP_PLUGIN)`

    PROTOS += ptzp/grpc/ptzp.proto

    protobuf_decl.name = protobuf headers
    protobuf_decl.input = PROTOS
    protobuf_decl.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.pb.h
    protobuf_decl.commands = protoc --cpp_out=${QMAKE_FILE_IN_PATH} --proto_path=${QMAKE_FILE_IN_PATH} ${QMAKE_FILE_NAME}
    protobuf_decl.variable_out = HEADERS
    QMAKE_EXTRA_COMPILERS += protobuf_decl

    #proto.pri
    protobuf_impl.name = protobuf sources
    protobuf_impl.input = PROTOS
    protobuf_impl.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.pb.cc
    protobuf_impl.depends = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.pb.h
    protobuf_impl.commands = $$escape_expand(\n)
    protobuf_impl.variable_out = SOURCES
    QMAKE_EXTRA_COMPILERS += protobuf_impl

    #grpc.pri
    grpc_decl.name = grpc headers
    grpc_decl.input = PROTOS
    grpc_decl.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.grpc.pb.h
    grpc_decl.commands = protoc --grpc_out=${QMAKE_FILE_IN_PATH} --plugin=protoc-gen-grpc=$(EXPORT_GRPC_CPP_PLUGIN_PATH) --proto_path=${QMAKE_FILE_IN_PATH} ${QMAKE_FILE_NAME}
    grpc_decl.variable_out = HEADERS
    QMAKE_EXTRA_COMPILERS += grpc_decl

    grpc_impl.name = grpc sources
    grpc_impl.input = PROTOS
    grpc_impl.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.grpc.pb.cc
    grpc_impl.depends = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.grpc.pb.h
    grpc_impl.commands = $$escape_expand(\n)
    grpc_impl.variable_out = SOURCES
    QMAKE_EXTRA_COMPILERS += grpc_impl
}
