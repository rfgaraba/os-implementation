/*
 * ELF executable loading
 * Copyright (c) 2003, Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 * Copyright (c) 2003, David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.29 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/errno.h>
#include <geekos/kassert.h>
#include <geekos/ktypes.h>
#include <geekos/screen.h>  /* for debug Print() statements */
#include <geekos/pfat.h>
#include <geekos/malloc.h>
#include <geekos/string.h>
#include <geekos/elf.h>


/**
 * Prints the contents of the ELF header
 * @param elfHeaderSection structure describing the Elf Header
 */
void Print_ELF_Header ( const elfHeader *header)
{
    Print("Elf Header\n");
    Print("ident: ");
    int i;
    for (i = 0; i<sizeof(header->ident)/sizeof(header->ident[0]); i++)
    {
        Print ("%x ", header->ident[i]);
    }
    Print ("\n");
    Print ("Type: %x\n", header->type);
    Print ("Machine: %x\n", header->machine);
    Print ("Version: %x\n", header->version);
    Print ("Entry: %x\n", header->entry);
    Print ("phoff: %x\n", header->phoff);
    Print ("sphoff: %x\n", header->sphoff);
    Print ("Flags: %x\n", header->flags);
    Print ("ehsize: %d\n", header->ehsize);
    Print ("phentsize: %x\n", header->phentsize);
    Print ("phnum: %x\n", header->phnum);
    Print ("shentsize: %x\n", header->shentsize);
    Print ("shnum: %x\n", header->shnum);
    Print ("shstrndx: %x\n", header->shstrndx);
   
}



/**
 * From the data of an ELF executable, extract the program header 
 * table
 * @param exeFileData buffer containing the executable file
 * @param exeFileLength length of the executable file in bytes
 * @param elfHeaderSection structure describing the Elf Header
 * @param programHeader structure array that will be created
 * @return 0 if successful, < 0 on error
 */
int Get_ELF_Program_Header_Table (const char *exeFileData, 
    ulong_t exeFileLength, elfHeader * elfHeaderSection, 
    programHeader * programHeaderTable)
{
    int retValue; /* Return Value */
    /* Program header is at the end of the elf header section*/
    const char *programTableAddress = exeFileData + elfHeaderSection->ehsize;

    int programHeaderTableSize = elfHeaderSection->phnum * 
        elfHeaderSection->phentsize;

    if (programHeaderTable != NULL && elfHeaderSection->phnum <= EXE_MAX_SEGMENTS )
    {   
        memcpy(programHeaderTable, programTableAddress, programHeaderTableSize);
        retValue = 0; 
    }
    else
    {
        retValue = EINVALID;
    }
    return retValue;
}

/**
 * From the data of an ELF executable, determine extract the header 
 * and check that it is an Executable
 * @param exeFileData buffer containing the executable file
 * @param exeFileLength length of the executable file in bytes
 * @param elfHeaderSection structure describing the Elf Header
 * @return 0 if successful, < 0 on error
 */
int Parse_ELF_Header(const char *exeFileData, ulong_t exeFileLength,
    elfHeader * elfHeaderSection)
{
    
    int retValue; /* Return Value */
    memcpy(elfHeaderSection, exeFileData, sizeof(elfHeader));
    /* Print_ELF_Header(elfHeaderSection); */
    if (elfHeaderSection->type == ET_EXEC)
    {
        if (elfHeaderSection->machine == EM_386)
        {   
            retValue = 0;
        }
        else
        {
            Print ("DBG: ELF file is not for 80386, machine type is %d\n",
                elfHeaderSection->type);
            retValue = ENOEXEC; 
        }
    }
    else
    {
        Print ("DBG: Elf File is not an Executable file\n");
        /* Invalid executable format */
        retValue = ENOEXEC; 
    }
   
    return 0;
}

/**
 * From the data of an ELF executable, determine how its segments
 * need to be loaded into memory.
 * @param exeFileData buffer containing the executable file
 * @param exeFileLength length of the executable file in bytes
 * @param exeFormat structure describing the executable's segments
 *   and entry address; to be filled in
 * @return 0 if successful, < 0 on error
 */
int Parse_ELF_Executable(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat)
{
    int retValue; /* Return Value */
    elfHeader * elfHeaderSection = NULL;
    programHeader programHeaderTable[EXE_MAX_SEGMENTS];

    if (exeFileData != NULL && exeFileLength != 0 && exeFormat != NULL)
    {
        retValue = Parse_ELF_Header(exeFileData, exeFileLength, 
            elfHeaderSection);
        if (retValue == 0)
        {   
            int i; 
            retValue = Get_ELF_Program_Header_Table(exeFileData, exeFileLength, 
                elfHeaderSection, programHeaderTable);
            
            if (retValue == 0)
            {
                exeFormat->entryAddr = elfHeaderSection->entry;
                exeFormat->numSegments = elfHeaderSection->phnum;
                for (i=0; i<elfHeaderSection->phnum; i++)
                {
                    exeFormat->segmentList[i].offsetInFile = 
                            programHeaderTable[i].offset;
                    exeFormat->segmentList[i].lengthInFile = 
                            programHeaderTable[i].fileSize;
                    exeFormat->segmentList[i].startAddress = 
                            programHeaderTable[i].vaddr;
                    exeFormat->segmentList[i].sizeInMemory = 
                            programHeaderTable[i].memSize;
                    exeFormat->segmentList[i].protFlags = 
                            programHeaderTable[i].flags;
                    
                }
            } 
        }
        /* else, retValue will be passed during return of function */
    }
    else
    {
        /* Invalid parameters */
        retValue = EINVALID;
    }
    return retValue;
}

