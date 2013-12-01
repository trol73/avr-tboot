/*
 * Copyright (c) 2009-2013 by Oleg Trifonov <otrifonow@gmail.com>
 *
 * http://trolsoft.ru
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include <cstdio>


#include <glog/logging.h>

#include "datafile.h"
#include "strutils.h"
#include "device.h"
#include "config.h"
#include "ui.h"
#include "filestream.h"




void usage() {
	printf(
		"t-bootloader v1.1.  Copyright 2009-2013 Oleg Trifonov\n"
		"Usage: tboot [options]\n"
		"Options:\n"
		"   -p <partno>                Specify AVR device.\n"
		"   -b <baudrate>              Specify serial port baud rate.\n"
		"   -C <config-file>           Specify location of configuration file.\n"
		"   -P <port>                  Specify connection port.\n"
		"   -U <flash|eeprom>:r|w|v:<filename.ext>\n"
		"                              Memory operation specification.\n"
		"                              Multiple -U options are allowed, each request\n"
		"                              is performed in the order specified.\n"
		"   -n                         Do not write anything to the device.\n"
		"   -V                         Do not verify.\n"
		"   -v                         Verbose output.\n"
		"   -q                         Quell progress output.\n"
	);
}



int main(int argc, char* argv[]) {
	Config *cfg = new Config();
	UI *ui = new UI(cfg);
	Device *dev = new Device(ui);



	if ( argc == 1 ) {
		usage();
		return 1;
	}
	if ( !cfg->ParseCommandLine(argc, argv) ) {
		return 1;
	}
	if ( !cfg->Load() ) {
		return 2;
	}
	if ( cfg->device.length() == 0 ) {
		ui->Error("Error: device name doesn't defined");
		return 1;
	}
	if ( !cfg->SelectDevice(cfg->device) ) {
		return 1;
	}

	dev->SetTimeouts(cfg->writeTimeout, cfg->readTimeout);
	dev->SetStartCommand(cfg->startCommand);

	google::InitGoogleLogging(argv[0]);
	FLAGS_log_dir = "logs";

	// Show all VLOG(m) messages for m less or equal the value of this flag.
	FLAGS_v = cfg->loggingLevel;

	// Copy log messages at or above this level to stderr in addition to logfiles.
	// The numbers of severity levels INFO, WARNING, ERROR, and FATAL are 0, 1, 2, and 3, respectively.
	FLAGS_stderrthreshold = 10;

	if ( !dev->Open(cfg->port.c_str(), cfg->baudrate) ) {
		return 3;
	}
	ui->Info("Device: %s", cfg->device_desc.c_str());


	for ( unsigned int attempt = 5; attempt != 0; attempt-- ) {	// TODO to configuration
		if ( dev->Reset() ) {
			break;
		}
		if ( attempt == 0 ) {
			ui->Error("bootloader not found");
			dev->Close();
			return 4;
		}
	}

	dev->SetPageSize(cfg->device_page_size);
	if ( cfg->no_write ) {
		ui->Warning("writing is disabled");
		dev->DisableWriting(true);
	}

	// iterate for tasks
	for ( size_t task = 0; task < cfg->operations.size(); task++ ) {
		Config::TOperation *op = cfg->operations[task];
		switch ( op->type ) {
			// reading
			case 'r': {
				unsigned char *data;
				if ( op->target == 'f' ) {
					ui->Info("reading flash input file \"%s\"", op->filename.c_str());
					data = new unsigned char[cfg->device_rom_size];
					if ( !dev->ReadAll(data, cfg->device_rom_size) ) {
						delete[] data;
						ui->Error("chip flash reading error");
						return 4;
					}
				} else if ( op->target == 'e' ) {
					ui->Info("reading EEPROM input file \"%s\"", op->filename.c_str());
					ui->Error("EEPROM not supported in this version");
					dev->Close();
					return 10;
				}
				if ( !DataFile::SaveFile(op->filename, data, cfg->device_rom_size) )  {
					dev->Close();
					return 5;
				}
				delete[] data;
			}

			ui->Info("UART read: bytes = %i  time = %f sec \t rate = %f bps", dev->GetTotalBytesIn(), dev->GetTotalReadTime()/1e6, 1.0*dev->GetTotalBytesIn()/dev->GetTotalReadTime()*1e6);
			ui->Info("UART write: bytes = %i  time = %f sec \t rate = %f bps", dev->GetTotalBytesOut(), dev->GetTotalWriteTime()/1e6, 1.0*dev->GetTotalBytesOut()/dev->GetTotalWriteTime()*1e6);

				break;
			// writing
			case 'w': {
				DataFile dataFile;

				ui->Info("reading input file \"%s\"\n", op->filename.c_str());

				if ( !dataFile.Load(op->filename) )  {
					dev->Close();
					return 5;
				}
				if ( op->target == 'f' ) {
					ui->Info("writing flash (%i bytes)", dataFile.size);
					if ( cfg->smart ) {
						ui->Info("smart-mode enabled");
					}
					// chip flash size verification
					int loaderOffset = dev->GetLoaderOffset();
					if (loaderOffset < 0) {
						ui->Error("error! can't get bootloader offset");
						dev->Close();
						return 6;
					}
					if ( dataFile.size > loaderOffset ) {
						// TODO !!! more detail info here
						ui->Error("error! data too large: available %i bytes", loaderOffset);
						dev->Close();
						return 6;
					}
					// preparing a data array
					int *data = new int[dataFile.size];
					for ( int i = 0; i < dataFile.size; i++ ) {
						data[i] = dataFile.data[i];
					}
					bool writeResult = dev->WriteAll(data, dataFile.size, cfg->smart);
					delete[] data;
					if ( !writeResult ) {
						printf("\n");
						ui->Error("chip flash write error");
						dev->Close();
						return 4;
					}
					if ( cfg->smart ) {
						ui->Info("smart mode: %i pages from %i have been rewritten (%.2f%%)", dev->GetPagesWrited(), dev->GetPagesForWriteTotal(), 100.0*dev->GetPagesWrited()/dev->GetPagesForWriteTotal());
					}
				} else if ( op->target == 'e' ) {
					ui->Error("EEPROM doesn't supported for this version");
					dev->Close();
					return 10;
				}
			}
			ui->Info("UART read: bytes = %i  time = %f sec \t rate = %f bps", dev->GetTotalBytesIn(), dev->GetTotalReadTime()/1e6, 1.0*dev->GetTotalBytesIn()/dev->GetTotalReadTime()*1e6);
			ui->Info("UART write: bytes = %i  time = %f sec \t rate = %f bps", dev->GetTotalBytesOut(), dev->GetTotalWriteTime()/1e6, 1.0*dev->GetTotalBytesOut()/dev->GetTotalWriteTime()*1e6);

			if ( !cfg->verify ) {
				break;
			}

			// verifying
			case 'v': {
				DataFile dataFile;

				if ( !dataFile.Load(op->filename) ) {
					dev->Close();
					return 5;
				}
				unsigned char *readed = NULL;
				size_t readedSize = 0;

				if ( op->target == 'f' ) {
					ui->Info("verifying flash against \"%s\"", op->filename.c_str());

					readedSize = cfg->device_rom_size;
					readed = new unsigned char[readedSize];
					if ( !dev->ReadAll(readed, cfg->device_rom_size) ) {
						delete[] readed;
						ui->Error("chip flash reading error");
						dev->Close();
						return 4;
					}
				} else if ( op->target == 'e' ) {
					ui->Error("EEPROM doesn't supported for this version");
					dev->Close();
					return 10;
				}
				if ( dataFile.size > readedSize ) {
					ui->Error("verification error, source file to large");
				}
				size_t len = dataFile.size < readedSize ? dataFile.size : readedSize;
				bool equals = true;
				for ( size_t i = 0; i < len; i++) {
					if ( i < readedSize && dataFile.data[i] >= 0 && dataFile.data[i] != readed[i] ) {
						ui->Error("verification error, address=%s chip=%s file=%s\n", HexToStr(i, 2).c_str(), HexToStr(readed[i], 1).c_str(),HexToStr(dataFile.data[i] , 1).c_str());
						equals = false;
					}
				}
				ui->Message("%i bytes of %s verified\n", dataFile.definedBytes, op->target == 'f' ? "flash" : "eeprom");
				delete[] readed;
			}

			ui->Info("UART read: bytes = %i  time = %f sec \t rate = %f bps", dev->GetTotalBytesIn(), dev->GetTotalReadTime()/1e6, 1.0*dev->GetTotalBytesIn()/dev->GetTotalReadTime()*1e6);
			ui->Info("UART write: bytes = %i  time = %f sec \t rate = %f bps", dev->GetTotalBytesOut(), dev->GetTotalWriteTime()/1e6, 1.0*dev->GetTotalBytesOut()/dev->GetTotalWriteTime()*1e6);

			break;
		} // switch (task)
		ui->Info("");
	} // for ( task )


	if ( !dev->SendCommandStr(cfg->finishCommand) ) {
		LOG(ERROR) << "can't send finish command " << cfg->finishCommand;
		dev->Close();
		return 100;
	}
	
	
	dev->GetLoaderOffset();
	
	dev->Close();
	

	return 0;
}
