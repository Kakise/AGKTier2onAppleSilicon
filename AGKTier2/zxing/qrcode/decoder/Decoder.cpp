/*
 *  Decoder.cpp
 *  zxing
 *
 *  Created by Christian Brunschen on 20/05/2008.
 *  Copyright 2008 ZXing authors All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <qrcode/decoder/Decoder.h>
#include <qrcode/decoder/BitMatrixParser.h>
#include <qrcode/ErrorCorrectionLevel.h>
#include <qrcode/Version.h>
#include <qrcode/decoder/DataBlock.h>
#include <qrcode/decoder/DecodedBitStreamParser.h>
#include <ReaderException.h>
#include <common/reedsolomon/ReedSolomonException.h>

//namespace zxing {
//namespace qrcode {

using namespace std; 
using namespace zxing;
using namespace zxing::qrcode;

Decoder::Decoder() : rsDecoder_(GF256::QR_CODE_FIELD) {
}

void Decoder::correctErrors(ArrayRef<unsigned char> codewordBytes, int numDataCodewords) {
  int numCodewords = (int)codewordBytes->size();
  ArrayRef<int> codewordInts(numCodewords);
  for (int i = 0; i < numCodewords; i++) {
    codewordInts[i] = codewordBytes[i] & 0xff;
  }
  int numECCodewords = numCodewords - numDataCodewords;

  try {
    rsDecoder_.decode(codewordInts, numECCodewords);
  } catch (ReedSolomonException const& ex) {
    ReaderException rex(ex.what());
    throw rex;
  }

  for (int i = 0; i < numDataCodewords; i++) {
    codewordBytes[i] = (unsigned char)codewordInts[i];
  }
}

Ref<DecoderResult> Decoder::decode(Ref<BitMatrix> bits) {
  // Construct a parser and read version, error-correction level
  BitMatrixParser parser(bits);

  Version *version = parser.readVersion();
  ErrorCorrectionLevel &ecLevel = parser.readFormatInformation()->getErrorCorrectionLevel();


  // Read codewords
  ArrayRef<unsigned char> codewords(parser.readCodewords());


  // Separate into data blocks
  std::vector<Ref<DataBlock> > dataBlocks(DataBlock::getDataBlocks(codewords, version, ecLevel));


  // Count total number of data bytes
  int totalBytes = 0;
  for (size_t i = 0; i < dataBlocks.size(); i++) {
    totalBytes += dataBlocks[i]->getNumDataCodewords();
  }
  ArrayRef<unsigned char> resultBytes(totalBytes);
  int resultOffset = 0;


  // Error-correct and copy data blocks together into a stream of bytes
  for (size_t j = 0; j < dataBlocks.size(); j++) {
    Ref<DataBlock> dataBlock(dataBlocks[j]);
    ArrayRef<unsigned char> codewordBytes = dataBlock->getCodewords();
    int numDataCodewords = dataBlock->getNumDataCodewords();
    correctErrors(codewordBytes, numDataCodewords);
    for (int i = 0; i < numDataCodewords; i++) {
      resultBytes[resultOffset++] = codewordBytes[i];
    }
  }

  return DecodedBitStreamParser::decode(resultBytes,
                                        version,
                                        ecLevel,
                                        DecodedBitStreamParser::Hashtable());
}


