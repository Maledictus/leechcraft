/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2011  Alexander Konovalov
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "cryptosystem.h"
#include <QByteArray>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <util/sll/util.h>
#include "ciphertextformat.h"

namespace LeechCraft
{
namespace SecMan
{
namespace SecureStorage
{
	const char* WrongHMACException::what () const throw ()
	{
		return "WrongHMACException";
	}

	CryptoSystem::CryptoSystem (const QString& password)
	{
		Key_ = CreateKey (password);
	}

	CryptoSystem::~CryptoSystem ()
	{
		Key_.fill (0, Key_.length ());
	}

	namespace
	{
		void Check (int retCode)
		{
			if (retCode)
				return;

			const char *file = nullptr;
			int line = 0;
			const char *data = nullptr;
			int flags = ERR_TXT_STRING;
			ERR_get_error_line_data (&file, &line, &data, &flags);
			throw std::runtime_error { QString { "encryption failed: %1 at %2:%3 (%4)" }
					.arg (data)
					.arg (file)
					.arg (line)
					.arg (retCode)
					.toStdString () };
		}
	}

	QByteArray CryptoSystem::Encrypt (const QByteArray& data) const
	{
		QByteArray result;
		result.resize (CipherTextFormatUtils::BufferLengthFor (data.length ()));
		CipherTextFormat cipherText (result.data (), data.length ());

		// fill IV in cipherText & random block
		Check (RAND_bytes (cipherText.Iv (), IVLength));
		unsigned char randomData [RndLength];
		Check (RAND_bytes (randomData, RndLength));

		// init cipher
		EVP_CIPHER_CTX cipherCtx;
		EVP_CIPHER_CTX_init (&cipherCtx);
		Check (EVP_EncryptInit (&cipherCtx, EVP_aes_256_ofb (),
				reinterpret_cast<const unsigned char*> (Key_.data ()),
				cipherText.Iv ()));

		// encrypt
		int outLength = 0;
		unsigned char* outPtr = cipherText.Data ();
		Check (EVP_EncryptUpdate (&cipherCtx, outPtr, &outLength,
				reinterpret_cast<const unsigned char*> (data.data ()),
				data.length ()));
		outPtr += outLength;
		Check (EVP_EncryptUpdate (&cipherCtx, outPtr, &outLength, randomData, RndLength));
		outPtr += outLength;
		// output last block & cleanup
		Check (EVP_EncryptFinal (&cipherCtx, outPtr, &outLength));

		// compute hmac
		{
			HMAC_CTX hmacCtx;
			HMAC_CTX_init (&hmacCtx);
			auto cleanup = Util::MakeScopeGuard ([&hmacCtx] { HMAC_CTX_cleanup (&hmacCtx); });
			Check (HMAC_Init_ex (&hmacCtx, Key_.data (), Key_.length (), EVP_sha256 (), 0));
			Check (HMAC_Update (&hmacCtx, reinterpret_cast<const unsigned char*> (data.data ()), data.length ()));
			Check (HMAC_Update (&hmacCtx, randomData, RndLength));
			unsigned hmacLength = 0;
			Check (HMAC_Final (&hmacCtx, cipherText.Hmac (), &hmacLength));
		}

		return result;
	}

	QByteArray CryptoSystem::Decrypt (const QByteArray& cipherText) const
	{
		if (CipherTextFormatUtils::DataLengthFor (cipherText.length ()) < 0)
			throw WrongHMACException ();

		QByteArray data;
		data.resize (CipherTextFormatUtils::DecryptBufferLengthFor (cipherText.length ()));
		CipherTextFormat cipherTextFormat (const_cast<char*> (cipherText.data ()),
				CipherTextFormatUtils::DataLengthFor (cipherText.length ()));

		// init cipher
		EVP_CIPHER_CTX cipherCtx;
		EVP_CIPHER_CTX_init (&cipherCtx);
		EVP_DecryptInit (&cipherCtx, EVP_aes_256_ofb (),
				reinterpret_cast<const unsigned char*> (Key_.data ()),
				cipherTextFormat.Iv ());

		// decrypt
		int outLength = 0;
		unsigned char* outPtr = reinterpret_cast<unsigned char*> (data.data ());
		EVP_DecryptUpdate (&cipherCtx, outPtr, &outLength,
				cipherTextFormat.Data (), cipherTextFormat.GetDataLength ());
		outPtr += outLength;
		EVP_DecryptUpdate (&cipherCtx, outPtr, &outLength,
				cipherTextFormat.Rnd (), RndLength);
		outPtr += outLength;
		// output last block & cleanup
		EVP_DecryptFinal (&cipherCtx, outPtr, &outLength);

		// compute hmac
		unsigned char hmac [HMACLength];
		HMAC_CTX hmacCtx;
		HMAC_CTX_init (&hmacCtx);
		HMAC_Init_ex (&hmacCtx, Key_.data (), Key_.length (),
				EVP_sha256 (), 0);
		HMAC_Update (&hmacCtx, reinterpret_cast<unsigned char*> (data.data ()), data.length ());
		unsigned int hmacLength = 0;
		HMAC_Final (&hmacCtx, hmac, &hmacLength);
		HMAC_CTX_cleanup (&hmacCtx);

		// check hmac
		const bool hmacsDifferent = memcmp (hmac, cipherTextFormat.Hmac (), HMACLength);
		if (hmacsDifferent)
			throw WrongHMACException ();

		// remove random block
		data.truncate (cipherTextFormat.GetDataLength ());

		return data;
	}

	QByteArray CryptoSystem::Hash (const QByteArray& data) const
	{
		unsigned char hash [HashLength];
		SHA256 (reinterpret_cast<const unsigned char*> (data.data ()), data.size (), hash);
		return QByteArray (reinterpret_cast<char*> (hash), HashLength);
	}

	QByteArray CryptoSystem::CreateKey (const QString& password) const
	{
		QByteArray res = Hash (password.toUtf8 ());
		res.resize (KeyLength);
		return res;
	}
}
}
}
