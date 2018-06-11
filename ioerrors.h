#ifndef IOERRORS_H
#define IOERRORS_H

/*
 * if you delete any entry from this enum,
 * please also update ioErrorStr array
 */
enum IOErr {
	IOE_NONE,
	IOE_UNSUPP_SPECIAL,
	IOE_SPECIAL_CHKSUM,
	IOE_UNSUPP_COMMAND,
	IOE_UNSUPP_VISCA_COMMAND,
	IOE_PELCOD_CHKSUM,
	IOE_SPECIAL_LAST,
	IOE_VISCA_LAST,
	IOE_NO_LAST_WRITTEN,
	IOE_VISCA_INVALID_ADDR,
};

#endif // IOERRORS_H

