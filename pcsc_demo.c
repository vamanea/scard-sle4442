/*
 * Sample program to use PC/SC API.
 *
 * MUSCLE SmartCard Development ( http://pcsclite.alioth.debian.org/pcsclite.html )
 *
 * Copyright (C) 2003-2011
 *  Ludovic Rousseau <ludovic.rousseau@free.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * $Id: pcsc_demo.c 6851 2014-02-14 15:43:32Z rousseau $
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#include <scard3w.h>


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* PCSC error message pretty print */
#define PCSC_ERROR(rv, text) \
if (rv != SCARD_S_SUCCESS) \
{ \
	fprintf(stderr, text ": %s (0x%lX)\n", pcsc_stringify_error(rv), rv); \
	goto end; \
} \
else \
{ \
	fprintf(stderr,text ": OK\n\n"); \
}

int main(int argc, char *argv[])
{
	LONG rv;
	SCARDCONTEXT hContext;
	DWORD dwReaders;
	LPSTR mszReaders = NULL;
	char *ptr, **readers = NULL;
	int nbReaders;
	SCARDHANDLE hCard;
	DWORD dwActiveProtocol, dwReaderLen, dwState, dwProt, dwAtrLen;
	BYTE pbAtr[MAX_ATR_SIZE] = "";
	char pbReader[MAX_READERNAME] = "";
	int reader_nb;
	unsigned int i;
	const SCARD_IO_REQUEST *pioSendPci;
	SCARD_IO_REQUEST pioRecvPci;
	BYTE pbRecvBuffer[256];
	BYTE pbPin[3];
	FILE *fInput;
	OKERR eErr = NO_ERROR;

	DWORD dwSendLength, dwRecvLength, dwAddress;

	fprintf(stderr,"PC/SC sample code\n");

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		fprintf(stderr,"SCardEstablishContext: Cannot Connect to Resource Manager %lX\n", rv);
		return EXIT_FAILURE;
	}

	/* Retrieve the available readers list. */
	dwReaders = SCARD_AUTOALLOCATE;
	rv = SCardListReaders(hContext, NULL, (LPSTR)&mszReaders, &dwReaders);
	PCSC_ERROR(rv, "SCardListReaders")

	/* Extract readers from the null separated string and get the total
	 * number of readers */
	nbReaders = 0;
	ptr = mszReaders;
	while (*ptr != '\0')
	{
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	if (nbReaders == 0)
	{
		fprintf(stderr,"No reader found\n");
		goto end;
	}

	/* allocate the readers table */
	readers = calloc(nbReaders, sizeof(char *));
	if (NULL == readers)
	{
		fprintf(stderr,"Not enough memory for readers[]\n");
		goto end;
	}

	/* fill the readers table */
	nbReaders = 0;
	ptr = mszReaders;
	while (*ptr != '\0')
	{
		fprintf(stderr,"%d: %s\n", nbReaders, ptr);
		readers[nbReaders] = ptr;
		ptr += strlen(ptr)+1;
		nbReaders++;
	}

	if (argc > 1)
	{
		reader_nb = atoi(argv[1]);
		if (reader_nb < 0 || reader_nb >= nbReaders)
		{
			fprintf(stderr,"Wrong reader index: %d\n", reader_nb);
			goto end;
		}
	}
	else
		reader_nb = 0;

	/* connect to a card */
	dwActiveProtocol = -1;
	rv = SCardConnect(hContext, readers[reader_nb], SCARD_SHARE_SHARED,
		SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
	fprintf(stderr," Protocol: %ld\n", dwActiveProtocol);
	PCSC_ERROR(rv, "SCardConnect")

	/* get card status */
	dwAtrLen = sizeof(pbAtr);
	dwReaderLen = sizeof(pbReader);
	rv = SCardStatus(hCard, /*NULL*/ pbReader, &dwReaderLen, &dwState, &dwProt,
		pbAtr, &dwAtrLen);
	fprintf(stderr," Reader: %s (length %ld bytes)\n", pbReader, dwReaderLen);
	fprintf(stderr," State: 0x%lX\n", dwState);
	fprintf(stderr," Prot: %ld\n", dwProt);
	fprintf(stderr," ATR (length %ld bytes):", dwAtrLen);
	for (i=0; i<dwAtrLen; i++)
		fprintf(stderr," %02X", pbAtr[i]);
	fprintf(stderr,"\n");
	PCSC_ERROR(rv, "SCardStatus")

	switch(dwActiveProtocol)
	{
		case SCARD_PROTOCOL_T0:
			pioSendPci = SCARD_PCI_T0;
			break;
		case SCARD_PROTOCOL_T1:
			pioSendPci = SCARD_PCI_T1;
			break;
		default:
			fprintf(stderr,"Unknown protocol\n");
			goto end;
	}

	dwRecvLength = 0x20;
	dwAddress = 0x00;
	eErr = SCard2WBPReadData(hCard,
							dwRecvLength,
							pbRecvBuffer,
							dwAddress);
	if (eErr != NO_ERROR) {
		fprintf(stderr,"ReadData returned error: %08x\n", (unsigned int)eErr);
		goto end;
	}
	fprintf(stderr,"Received: ");
	for (i = 0; i < dwRecvLength; i++) {
		printf("%c", pbRecvBuffer[i]);
	}

	for (i = 0; i < dwRecvLength; i++) {
		fprintf(stderr,"%02X ", pbRecvBuffer[i]);
	}
	fprintf(stderr,"\n");

	pbPin[0] = 0xFF;
	pbPin[1] = 0xFF;
	pbPin[2] = 0xFF;

	eErr = SCard2WBPPresentPIN(hCard,
							sizeof(pbPin),
							pbPin);
	if (eErr != NO_ERROR) {
		fprintf(stderr,"PresentPIN returned error: %08x\n", (unsigned int)eErr);
		goto end;
	}
	fprintf(stderr,"Pin OK!\n");
/*	fInput = fopen("keyfile", "rb");
	if (fInput == NULL) {
		fprintf(stderr,"Failed to open keyfile\n");
		goto end;
	}
	dwRecvLength = fread(pbRecvBuffer, 1, 256, fInput);

	eErr = SCard2WBPWriteData(hCard,
							dwRecvLength,
							pbRecvBuffer,
							dwAddress);
	if (eErr != NO_ERROR) {
		fprintf(stderr,"WriteData returned error: %08x\n", (unsigned int)eErr);
	}

	fclose(fInput);*/
end:
    /* card disconnect */
	rv = SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
	PCSC_ERROR(rv, "SCardDisconnect")

	/* free allocated memory */
	if (mszReaders)
		SCardFreeMemory(hContext, mszReaders);

	/* We try to leave things as clean as possible */
	rv = SCardReleaseContext(hContext);
	if (rv != SCARD_S_SUCCESS)
		fprintf(stderr,"SCardReleaseContext: %s (0x%lX)\n", pcsc_stringify_error(rv),
			rv);

	if (readers)
		free(readers);

	return EXIT_SUCCESS;
} /* main */

