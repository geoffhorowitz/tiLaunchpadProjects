/*
 * -------------------------------------------
 *    MSP432 DriverLib - v2_20_00_08 
 * -------------------------------------------
 *
 * --COPYRIGHT--,BSD,BSD
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#include <aes256.h>
#include <interrupt.h>
#include <debug.h>

bool AES256_setCipherKey(uint32_t moduleInstance, const uint8_t * cipherKey,
        uint_fast16_t keyLength)
{
    uint8_t i;
    uint16_t sCipherKey;

    AES256_CMSIS(moduleInstance)->rCTL0.r |= 0;

    switch (keyLength)
    {
    case AES256_KEYLENGTH_128BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__128BIT;
        break;

    case AES256_KEYLENGTH_192BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__192BIT;
        break;

    case AES256_KEYLENGTH_256BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__256BIT;
        break;
    default:
        return false;
    }

    keyLength = keyLength / 8;

    for (i = 0; i < keyLength; i = i + 2)
    {
        sCipherKey = (uint16_t) (cipherKey[i]);
        sCipherKey = sCipherKey | ((uint16_t) (cipherKey[i + 1]) << 8);
        AES256_CMSIS(moduleInstance)->rKEY.r = sCipherKey;
    }

    // Wait until key is written
    while (!BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESKEYWR_OFS))
        ;

    return true;
}

void AES256_encryptData(uint32_t moduleInstance, const uint8_t * data,
        uint8_t * encryptedData)
{
    uint8_t i;
    uint16_t tempData = 0;
    uint16_t tempVariable = 0;

    // Set module to encrypt mode
    AES256_CMSIS(moduleInstance)->rCTL0.r &= ~AESOP_M;

    // Write data to encrypt to module
    for (i = 0; i < 16; i = i + 2)
    {
        tempVariable = (uint16_t) (data[i]);
        tempVariable = tempVariable | ((uint16_t) (data[i + 1]) << 8);
        AES256_CMSIS(moduleInstance)->rDIN.r = tempVariable;
    }

    // Key that is already written shall be used
    // Encryption is initialized by setting AESKEYWR to 1
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESKEYWR_OFS) = 1;

    // Wait unit finished ~167 MCLK
    while (BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESBUSY_OFS))
        ;

    // Write encrypted data back to variable
    for (i = 0; i < 16; i = i + 2)
    {
        tempData = AES256_CMSIS(moduleInstance)->rDOUT.r;
        *(encryptedData + i) = (uint8_t) tempData;
        *(encryptedData + i + 1) = (uint8_t) (tempData >> 8);
    }
}

void AES256_decryptData(uint32_t moduleInstance, const uint8_t * data,
        uint8_t * decryptedData)
{
    uint8_t i;
    uint16_t tempData = 0;
    uint16_t tempVariable = 0;

    // Set module to decrypt mode
    AES256_CMSIS(moduleInstance)->rCTL0.r |= (AESOP_3);

    // Write data to decrypt to module
    for (i = 0; i < 16; i = i + 2)
    {
        tempVariable = (uint16_t) (data[i + 1] << 8);
        tempVariable = tempVariable | ((uint16_t) (data[i]));
        AES256_CMSIS(moduleInstance)->rDIN.r = tempVariable;
    }

    // Key that is already written shall be used
    // Now decryption starts
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESKEYWR_OFS) = 1;

    // Wait unit finished ~167 MCLK
    while (BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESBUSY_OFS))
        ;

    // Write encrypted data back to variable
    for (i = 0; i < 16; i = i + 2)
    {
        tempData = AES256_CMSIS(moduleInstance)->rDOUT.r;
        *(decryptedData + i) = (uint8_t) tempData;
        *(decryptedData + i + 1) = (uint8_t) (tempData >> 8);
    }
}

bool AES256_setDecipherKey(uint32_t moduleInstance, const uint8_t * cipherKey,
        uint_fast16_t keyLength)
{
    uint8_t i;
    uint16_t tempVariable = 0;

    // Set module to decrypt mode
    AES256_CMSIS(moduleInstance)->rCTL0.r =
            (AES256_CMSIS(moduleInstance)->rCTL0.r & ~AESOP_M) | AESOP1;

    switch (keyLength)
    {
    case AES256_KEYLENGTH_128BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__128BIT;
        break;

    case AES256_KEYLENGTH_192BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__192BIT;
        break;

    case AES256_KEYLENGTH_256BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__256BIT;
        break;

    default:
        return false;
    }

    keyLength = keyLength / 8;

    // Write cipher key to key register
    for (i = 0; i < keyLength; i = i + 2)
    {
        tempVariable = (uint16_t) (cipherKey[i]);
        tempVariable = tempVariable | ((uint16_t) (cipherKey[i + 1]) << 8);
        AES256_CMSIS(moduleInstance)->rKEY.r = tempVariable;
    }

    // Wait until key is processed ~52 MCLK
    while (BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESBUSY_OFS))
        ;

    return true;
}

void AES256_clearInterruptFlag(uint32_t moduleInstance)
{
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rCTL0.r,AESRDYIFG_OFS) = 0;
}

uint32_t AES256_getInterruptFlagStatus(uint32_t moduleInstance)
{
    return BITBAND_PERI(AES256_CMSIS(moduleInstance)->rCTL0.r, AESRDYIFG_OFS);
}

void AES256_enableInterrupt(uint32_t moduleInstance)
{
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rCTL0.r,AESRDYIE_OFS) = 1;
}

void AES256_disableInterrupt(uint32_t moduleInstance)
{
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rCTL0.r,AESRDYIE_OFS) = 0;
}

void AES256_reset(uint32_t moduleInstance)
{
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rCTL0.r,AESSWRST_OFS) = 1;
}

void AES256_startEncryptData(uint32_t moduleInstance, const uint8_t * data)
{
    uint8_t i;
    uint16_t tempVariable = 0;

    // Set module to encrypt mode
    AES256_CMSIS(moduleInstance)->rCTL0.r &= ~AESOP_M;

    // Write data to encrypt to module
    for (i = 0; i < 16; i = i + 2)
    {
        tempVariable = (uint16_t) (data[i]);
        tempVariable = tempVariable | ((uint16_t) (data[i + 1]) << 8);
        AES256_CMSIS(moduleInstance)->rDIN.r = tempVariable;
    }

    // Key that is already written shall be used
    // Encryption is initialized by setting AESKEYWR to 1
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESKEYWR_OFS) = 1;
}

void AES256_startDecryptData(uint32_t moduleInstance, const uint8_t * data)
{
    uint8_t i;
    uint16_t tempVariable = 0;

    // Set module to decrypt mode
    AES256_CMSIS(moduleInstance)->rCTL0.r |= (AESOP_3);

    // Write data to decrypt to module
    for (i = 0; i < 16; i = i + 2)
    {
        tempVariable = (uint16_t) (data[i + 1] << 8);
        tempVariable = tempVariable | ((uint16_t) (data[i]));
        AES256_CMSIS(moduleInstance)->rDIN.r = tempVariable;
    }

    // Key that is already written shall be used
    // Now decryption starts
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESKEYWR_OFS) = 1;
}

bool AES256_startSetDecipherKey(uint32_t moduleInstance,
        const uint8_t * cipherKey, uint_fast16_t keyLength)
{
    uint8_t i;
    uint16_t tempVariable = 0;

    AES256_CMSIS(moduleInstance)->rCTL0.r =
            (AES256_CMSIS(moduleInstance)->rCTL0.r & ~AESOP_M) | AESOP1;

    switch (keyLength)
    {
    case AES256_KEYLENGTH_128BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__128BIT;
        break;

    case AES256_KEYLENGTH_192BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__192BIT;
        break;

    case AES256_KEYLENGTH_256BIT:
        AES256_CMSIS(moduleInstance)->rCTL0.r |= AESKL__256BIT;
        break;

    default:
        return false;
    }

    keyLength = keyLength / 8;

    // Write cipher key to key register
    for (i = 0; i < keyLength; i = i + 2)
    {
        tempVariable = (uint16_t) (cipherKey[i]);
        tempVariable = tempVariable | ((uint16_t) (cipherKey[i + 1]) << 8);
        AES256_CMSIS(moduleInstance)->rKEY.r = tempVariable;
    }

    return true;
}

bool AES256_getDataOut(uint32_t moduleInstance, uint8_t *outputData)
{
    uint8_t i;
    uint16_t tempData = 0;

    // If module is busy, exit and return failure
    if (BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESBUSY_OFS))
        return false;

    // Write encrypted data back to variable
    for (i = 0; i < 16; i = i + 2)
    {
        tempData = AES256_CMSIS(moduleInstance)->rDOUT.r;
        *(outputData + i) = (uint8_t) tempData;
        *(outputData + i + 1) = (uint8_t) (tempData >> 8);
    }

    return true;
}

bool AES256_isBusy(uint32_t moduleInstance)
{
    return BITBAND_PERI(AES256_CMSIS(moduleInstance)->rSTAT.r, AESBUSY_OFS);
}

void AES256_clearErrorFlag(uint32_t moduleInstance)
{
    BITBAND_PERI(AES256_CMSIS(moduleInstance)->rCTL0.r, AESERRFG_OFS) = 0;
}

uint32_t AES256_getErrorFlagStatus(uint32_t moduleInstance)
{
    return BITBAND_PERI(AES256_CMSIS(moduleInstance)->rCTL0.r, AESERRFG_OFS);
}

void AES256_registerInterrupt(uint32_t moduleInstance, void (*intHandler)(void))
{
    Interrupt_registerInterrupt(INT_AES256, intHandler);
    Interrupt_enableInterrupt(INT_AES256);
}

void AES256_unregisterInterrupt(uint32_t moduleInstance)
{
    Interrupt_disableInterrupt(INT_AES256);
    Interrupt_unregisterInterrupt(INT_AES256);
}

uint32_t AES256_getInterruptStatus(uint32_t moduleInstance)
{
    return AES256_getInterruptFlagStatus(moduleInstance);
}

