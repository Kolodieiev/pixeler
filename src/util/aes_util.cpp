#include "aes_util.h"

#include <esp_random.h>

#include <cstring>

static constexpr size_t AES_KEY_BITS{256u};
static constexpr size_t AES_BLOCK_SIZE{16u};

namespace pixeler
{
  bool aes256Encrypt(const uint8_t* aes_key, const uint8_t* plain_data, const size_t plain_data_len, uint8_t* out_cipher_data)
  {
    if (const psa_status_t status = psa_crypto_init(); status != PSA_SUCCESS)
    {
      log_e("Помилка psa_crypto_init: %i", static_cast<int>(status));
      return false;
    }

    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, AES_KEY_BITS);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT);
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);

    psa_key_id_t key_id{PSA_KEY_ID_NULL};
    if (const psa_status_t status = psa_import_key(&attr, aes_key, AES_KEY_SIZE, &key_id); status != PSA_SUCCESS)
    {
      log_e("Помилка psa_import_key: %i", static_cast<int>(status));
      return false;
    }

    uint8_t iv[AES_IV_SIZE];
    uint8_t tag[AES_TAG_SIZE];
    esp_fill_random(iv, AES_IV_SIZE);

    psa_aead_operation_t op = PSA_AEAD_OPERATION_INIT;
    psa_status_t status = psa_aead_encrypt_setup(&op, key_id, PSA_ALG_GCM);
    if (status == PSA_SUCCESS)
      status = psa_aead_set_nonce(&op, iv, AES_IV_SIZE);

    uint8_t* cipher_out = out_cipher_data + AES_IV_SIZE + AES_TAG_SIZE;
    const size_t full_blocks_len = plain_data_len & ~(AES_BLOCK_SIZE - 1u);
    const size_t rem_len = plain_data_len - full_blocks_len;
    size_t total_out{0u};

    if (status == PSA_SUCCESS && full_blocks_len > 0u)
    {
      size_t out_len{0u};
      status = psa_aead_update(&op, plain_data, full_blocks_len, cipher_out, full_blocks_len, &out_len);
      total_out += out_len;
    }

    if (status == PSA_SUCCESS && rem_len > 0u)
    {
      uint8_t rem_buf[AES_BLOCK_SIZE];
      size_t out_len{0u};
      status = psa_aead_update(&op, plain_data + full_blocks_len, rem_len, rem_buf, sizeof(rem_buf), &out_len);
      if (status == PSA_SUCCESS && out_len > 0u)
      {
        if (total_out + out_len <= plain_data_len)
        {
          memcpy(cipher_out + total_out, rem_buf, out_len);
          total_out += out_len;
        }
        else
        {
          status = PSA_ERROR_BUFFER_TOO_SMALL;
        }
      }
    }

    if (status == PSA_SUCCESS)
    {
      uint8_t fin_buf[AES_BLOCK_SIZE];
      size_t fin_len{0u};
      size_t tag_len{0u};
      status = psa_aead_finish(&op, fin_buf, sizeof(fin_buf), &fin_len, tag, AES_TAG_SIZE, &tag_len);
      if (status == PSA_SUCCESS && fin_len > 0u)
      {
        if (total_out + fin_len <= plain_data_len)
        {
          memcpy(cipher_out + total_out, fin_buf, fin_len);
          total_out += fin_len;
        }
        else
        {
          status = PSA_ERROR_BUFFER_TOO_SMALL;
        }
      }

      if (status == PSA_SUCCESS && (total_out != plain_data_len || tag_len != AES_TAG_SIZE))
        status = PSA_ERROR_CORRUPTION_DETECTED;
    }

    if (status != PSA_SUCCESS)
    {
      psa_aead_abort(&op);
      psa_destroy_key(key_id);
      log_e("Encrypt err: %i", static_cast<int>(status));
      return false;
    }

    psa_destroy_key(key_id);

    memcpy(out_cipher_data, iv, AES_IV_SIZE);
    memcpy(out_cipher_data + AES_IV_SIZE, tag, AES_TAG_SIZE);

    return true;
  }

  bool aes256Decrypt(const uint8_t* aes_key, const uint8_t* cipher_data, const size_t plain_data_len, uint8_t* out_plain_data)
  {
    if (const psa_status_t status = psa_crypto_init(); status != PSA_SUCCESS)
    {
      log_e("Помилка psa_crypto_init: %i", static_cast<int>(status));
      return false;
    }

    psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attr, AES_KEY_BITS);
    psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attr, PSA_ALG_GCM);

    psa_key_id_t key_id{PSA_KEY_ID_NULL};
    if (const psa_status_t status = psa_import_key(&attr, aes_key, AES_KEY_SIZE, &key_id); status != PSA_SUCCESS)
    {
      log_e("Помилка psa_import_key: %i", static_cast<int>(status));
      return false;
    }

    const uint8_t* iv = cipher_data;
    const uint8_t* tag = cipher_data + AES_IV_SIZE;
    const uint8_t* cipher_in = cipher_data + AES_IV_SIZE + AES_TAG_SIZE;

    psa_aead_operation_t op = PSA_AEAD_OPERATION_INIT;
    psa_status_t status = psa_aead_decrypt_setup(&op, key_id, PSA_ALG_GCM);
    if (status == PSA_SUCCESS)
      status = psa_aead_set_nonce(&op, iv, AES_IV_SIZE);

    const size_t full_blocks_len = plain_data_len & ~(AES_BLOCK_SIZE - 1u);
    const size_t rem_len = plain_data_len - full_blocks_len;
    size_t total_out{0u};

    if (status == PSA_SUCCESS && full_blocks_len > 0u)
    {
      size_t out_len{0u};
      status = psa_aead_update(&op, cipher_in, full_blocks_len, out_plain_data, full_blocks_len, &out_len);
      total_out += out_len;
    }

    if (status == PSA_SUCCESS && rem_len > 0u)
    {
      uint8_t rem_buf[AES_BLOCK_SIZE];
      size_t out_len{0u};
      status = psa_aead_update(&op, cipher_in + full_blocks_len, rem_len, rem_buf, sizeof(rem_buf), &out_len);
      if (status == PSA_SUCCESS && out_len > 0u)
      {
        if (total_out + out_len <= plain_data_len)
        {
          memcpy(out_plain_data + total_out, rem_buf, out_len);
          total_out += out_len;
        }
        else
        {
          status = PSA_ERROR_BUFFER_TOO_SMALL;
        }
      }
    }

    if (status == PSA_SUCCESS)
    {
      uint8_t fin_buf[AES_BLOCK_SIZE];
      size_t fin_len{0u};
      status = psa_aead_verify(&op, fin_buf, sizeof(fin_buf), &fin_len, tag, AES_TAG_SIZE);
      if (status == PSA_SUCCESS && fin_len > 0u)
      {
        if (total_out + fin_len <= plain_data_len)
        {
          memcpy(out_plain_data + total_out, fin_buf, fin_len);
          total_out += fin_len;
        }
        else
        {
          status = PSA_ERROR_BUFFER_TOO_SMALL;
        }
      }

      if (status == PSA_SUCCESS && total_out != plain_data_len)
        status = PSA_ERROR_CORRUPTION_DETECTED;
    }

    if (status != PSA_SUCCESS)
    {
      psa_aead_abort(&op);
      psa_destroy_key(key_id);
      if (plain_data_len > 0u)
        memset(out_plain_data, 0, plain_data_len);
      log_e("Decrypt err: %i", static_cast<int>(status));
      return false;
    }

    psa_destroy_key(key_id);
    return true;
  }

  void generateAes256Key(uint8_t* out_aes_key_buff)
  {
    esp_fill_random(out_aes_key_buff, AES_KEY_SIZE);
  }
}  // namespace pixeler
