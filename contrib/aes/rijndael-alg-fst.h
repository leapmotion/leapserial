/**
 * rijndael-alg-fst.h
 *
 * @version 3.0 (December 2000)
 *
 * Optimised ANSI C code for the Rijndael cipher (now AES)
 *
 * @author Vincent Rijmen <vincent.rijmen@esat.kuleuven.ac.be>
 * @author Antoon Bosselaers <antoon.bosselaers@esat.kuleuven.ac.be>
 * @author Paulo Barreto <paulo.barreto@terra.com.br>
 *
 * This code is hereby placed in the public domain.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __RIJNDAEL_ALG_FST_H
#define __RIJNDAEL_ALG_FST_H
#include <stdint.h>

#define MAXKC	(256/32)
#define MAXKB	(256/8)
#define MAXNR	14

typedef struct _rijndael_context {
  // Number of initialized rounds
  int Nr;

  // Round keys for encryption
  uint32_t rke[4 * (MAXNR + 1)];

  // Round keys for decryption
  uint32_t rkd[4 * (MAXNR + 1)];
} rijndael_context;

#ifdef __cplusplus
extern "C" {
#endif

int rijndaelKeySetup(rijndael_context* rk, const uint8_t cipherKey[], int keyBits);
void rijndaelEncrypt(const rijndael_context* rk, const uint8_t pt[16], uint8_t ct[16]);
void rijndaelDecrypt(const rijndael_context* rk, const uint8_t ct[16], uint8_t pt[16]);

#ifdef __cplusplus
}
#endif

#endif /* __RIJNDAEL_ALG_FST_H */