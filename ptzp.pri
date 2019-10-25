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
    ptzp/tbgthdriver.h \
    ptzp/evpupthead.h \
    ptzp/mgeofalconeyehead.h \
    ptzp/kayidriver.h \
    ptzp/mgeoyamgozhead.h \
    ptzp/yamgozdriver.h \
    ptzp/flirpthead.h \
    ptzp/flirdriver.h \
    ptzp/flirmodulehead.h \
    ptzp/mgeoswirhead.h \
    ptzp/swirdriver.h \
    ptzp/virtualptzpdriver.h \
    ptzp/ptzphttptransport.h \
    ptzp/htrswirpthead.h \
    ptzp/htrswirdriver.h \
    ptzp/htrswirmodulehead.h \
    ptzp/moxacontrolhead.h \
    ptzp/flirpttcphead.h

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
    ptzp/tbgthdriver.cpp \
    ptzp/evpupthead.cpp \
    ptzp/mgeofalconeyehead.cpp \
    ptzp/kayidriver.cpp \
    ptzp/mgeoyamgozhead.cpp \
    ptzp/yamgozdriver.cpp \
    ptzp/flirpthead.cpp \
    ptzp/flirdriver.cpp \
    ptzp/flirmodulehead.cpp \
    ptzp/mgeoswirhead.cpp \
    ptzp/swirdriver.cpp \
    ptzp/virtualptzpdriver.cpp \
    ptzp/ptzphttptransport.cpp \
    ptzp/htrswirpthead.cpp \
    ptzp/htrswirdriver.cpp \
    ptzp/htrswirmodulehead.cpp \
    ptzp/moxacontrolhead.cpp \
    ptzp/flirpttcphead.cpp

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
    grpc_decl.depends = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.pb.h
    grpc_decl.variable_out = HEADERS
    QMAKE_EXTRA_COMPILERS += grpc_decl

    grpc_impl.name = grpc sources
    grpc_impl.input = PROTOS
    grpc_impl.output = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.grpc.pb.cc
    grpc_impl.depends = ${QMAKE_FILE_IN_PATH}/${QMAKE_FILE_BASE}.grpc.pb.h
    grpc_impl.commands = $$escape_expand(\n)
    grpc_impl.variable_out = SOURCES
    QMAKE_EXTRA_COMPILERS += grpc_impl

    grpc_files.files += ptzp/grpc/ptzp.grpc.pb.h ptzp/grpc/ptzp.pb.h
    grpc_files.path = $$INSTALL_PREFIX/usr/local/include/ecl/ptzp/grpc/
    INSTALLS += grpc_files
}
