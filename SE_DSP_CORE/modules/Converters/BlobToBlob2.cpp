/* Copyright (c) 2007-2023 SynthEdit Ltd

Permission to use, copy, modify, and /or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS.IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include <memory>
#include "mp_sdk_audio.h"
#include "SharedBlob.h"

SE_DECLARE_INIT_STATIC_FILE(BlobToBlob2);
SE_DECLARE_INIT_STATIC_FILE(Blob2ToBlob);

using namespace gmpi;

class BlobToBlob2 final : public MpBase2
{
	BlobInPin pinValueIn;
	Blob2OutPin pinValueOut;

	struct blobInfo
	{
		SharedBlobView blobview;
		std::string blobData;
	};

	// we're using unique_ptr because we MUST not accidentally change the address of a BLOB once we've sent it.
	std::vector<std::unique_ptr<blobInfo>> outputBlobPool;

public:
	BlobToBlob2()
	{
		initializePin(pinValueIn);
		initializePin(pinValueOut);
	}

	void onSetPins() override
	{
		// Allocate a BLOB
		blobInfo* blob = nullptr;

		for (size_t i = 0; i < std::size(outputBlobPool); ++i)
		{
			if (!outputBlobPool[i]->blobview.inUse())
			{
				blob = outputBlobPool[i].get();
				break;
			}
		}

		// If none free, make a new one.
		if (!blob)
		{
			outputBlobPool.push_back(std::make_unique<blobInfo>());
			blob = outputBlobPool.back().get();
		}

		// save the blob data into a string
		blob->blobData.assign(pinValueIn.getValue().getData(), pinValueIn.getValue().getSize());

		// associate the memory with the blob view
		blob->blobview.set(blob->blobData.data(), blob->blobData.size());

		// send my blob out
		pinValueOut = &blob->blobview;
	}
};

class Blob2ToBlob final : public MpBase2
{
	Blob2InPin pinValueIn;
	BlobOutPin pinValueOut;

public:
	Blob2ToBlob()
	{
		initializePin(pinValueIn);
		initializePin(pinValueOut);
	}

	void onSetPins() override
	{
		auto inputBlob = pinValueIn.getValue();

		uint8_t* data{};
		int64_t size{};
		if (inputBlob) // blob can be null
		{
			inputBlob->read(&data, &size);
		}
		pinValueOut.setValueRaw(static_cast<size_t>(size), data);
		pinValueOut.sendPinUpdate();
	}
};

namespace
{
	auto r1 = Register<BlobToBlob2>::withId(L"SE BlobToBlob2");
	auto r2 = Register<Blob2ToBlob>::withId(L"SE Blob2ToBlob");
}
