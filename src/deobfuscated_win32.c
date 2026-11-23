#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <mmsystem.h>
#include "apu.h"
#include "audio_win32.h"
#ifdef SMOLNES_ENABLE_7Z_LZMA1
#include "lzmacapi/SRC/sevenzip_single.h"
#endif

#ifdef SMOLNES_ENABLE_7Z_LZMA1
#define ROM_BUFFER_SIZE (1024 * 1024 + 16)
#else
#define ROM_BUFFER_SIZE 1024 * 1024
#endif

#define PULL mem(++S, 1, 0, 0)
#define PUSH(x) mem(S--, 1, x, 1)

uint8_t *rom, *chrrom,                // Points to the start of PRG/CHR ROM
    prg[4], chr[8],                   // Current PRG/CHR banks
    prgbits = 14, chrbits = 12,       // Number of bits per PRG/CHR bank
    A, X, Y, P = 4, S = ~2, pc_hi, PCL, // CPU Registers
    addr_lo, addr_hi,                 // Current instruction address
    nomem,  // 1 => current instruction doesn't write to memory
    result, // Temp variable
    val,    // Current instruction value
    cross,  // 1 => page crossing occurred
    tmp,    // Temp variables
    ppumask, ppuctrl, ppustatus, // PPU registers
    ppubuf,                      // PPU buffered reads
    W,                           // Write toggle PPU register
    fine_x,                      // X fine scroll offset, 0..7
    opcode,                      // Current instruction opcode
    nmi_irq,                     // 1 => IRQ occurred
                                 // 4 => NMI occurred
    ntb,                         // Nametable byte
    ptb_lo,                      // Pattern table lowbyte
    vram[2048],                  // Nametable RAM
    palette_ram[64],             // Palette RAM
    ram[8192],                   // CPU RAM
    chrram[8192],                // CHR RAM (only used for some games)
    prgram[8192],                // PRG RAM (only used for some games)
    oam[256],                    // Object Attribute Memory (sprite RAM)
    mask[] = {128, 64, 1, 2,     // Masks used in branch instructions
              1,   0,  0, 1, 4, 0, 0, 4, 0,
              0,   64, 0, 8, 0, 0, 8}, // Masks used in SE*/CL* instructions.
    keys,                              // Joypad shift register
    mirror,                            // Current mirroring mode
    mmc1_bits, mmc1_data, mmc1_ctrl,   // Mapper 1 (MMC1) registers
    mmc3_chrprg[8], mmc3_bits,         // Mapper 4 (MMC3) registers
    mmc3_irq, mmc3_latch,              //
    chrbank0, chrbank1, prgbank,       // Current PRG/CHR bank
    rombuf[ROM_BUFFER_SIZE];             // Buffer to read/decompress ROM into

#ifdef SMOLNES_ENABLE_7Z_LZMA1
static uint8_t *rom_header;
static uint8_t rom_archive[ROM_BUFFER_SIZE];

/* The supplied x86 decoder object was built with /GS. Retain the cookie check
   without linking the CRT's much larger failure-reporting machinery. */
uintptr_t __security_cookie = 0xBB40E64E;
void __fastcall __security_check_cookie(uintptr_t cookie) {
  if (cookie != __security_cookie)
    ExitProcess(ERROR_INVALID_DATA);
}
#else
#define rom_header rombuf
#endif

static HWND main_window;
static int app_running = 1;
static HANDLE frame_timer_16, frame_timer_50;
static unsigned frame_wait_count;
#ifdef SMOLNES_HEADLESS_TRACE
static unsigned headless_frame;
static FILE *headless_pcm_log;
static FILE *headless_mix_pcm;
static FILE *headless_dmc_pcm;
static FILE *headless_apu_time_log;
static uint64_t headless_audio_last_apu_cycle;
static uint64_t headless_audio_cycle_remainder;
static long long headless_frame_audio_energy;
static unsigned headless_frame_audio_peak;
#endif
#if defined(SMOLNES_FRAME_TIME_DIAGNOSTICS) || defined(SMOLNES_PACING_STRESS_TEST)
static LARGE_INTEGER frame_frequency;
#endif

#ifdef SMOLNES_FRAME_TIME_DIAGNOSTICS
static FILE *frame_timing_log;
static LARGE_INTEGER frame_work_start;
static LONGLONG frame_work_sum, frame_work_min, frame_work_max;
static LONGLONG frame_paint_sum, frame_paint_min, frame_paint_max;
static unsigned frame_work_samples;
static int frame_work_started;
#endif

#ifdef SMOLNES_FPS_DIAGNOSTICS
static DWORD fps_sample_start;
static unsigned fps_sample_frames;
#endif

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

typedef struct {
  BITMAPINFOHEADER header;
  DWORD masks[3];
} FrameBitmapInfo;

static const FrameBitmapInfo frame_bitmap_info = {
    {sizeof(BITMAPINFOHEADER), 256, -224, 1, 16, BI_BITFIELDS, 0, 0, 0, 0, 0},
    {0x001F, 0x07E0, 0xF800}};

static uint8_t scanline_sprite_indices[8];
static uint8_t scanline_sprite_rows[8];
static uint8_t scanline_sprite_count;
static uint8_t scanline_sprite_height;
static uint8_t scanline_background_color[256];
static uint8_t scanline_background_palette[256];
static uint8_t scanline_sprite_pixel_color[256];
static uint8_t scanline_sprite_pixel_attributes[256];
static uint8_t scanline_sprite_pixel_owner[256];

uint8_t *get_chr_byte(uint16_t a);

static const uint16_t nes_palette_rgb565[64] = {
    25356, 34816, 39011, 30854, 24714, 4107,  106,   2311,
    2468,  2561,  4642,  6592,  20832, 0,     0,     0,
    44373, 49761, 55593, 51341, 43186, 18675, 434,   654,
    4939,  5058,  3074,  19362, 37667, 0,     0,     0,
    ~0,    ~819,  64497, 64342, 62331, 43932, 23612, 9465,
    1429,  1550,  20075, 36358, 52713, 16904, 0,     0,
    ~0,    ~328,  ~422,  ~452,  ~482,  58911, 50814, 42620,
    40667, 40729, 48951, 53078, 61238, 44405};

uint16_t scany,          // Scanline Y
    T, V,                // "Loopy" PPU registers
    sum,                 // Sum used for ADC/SBC
    dot,                 // Horizontal position of PPU, from 0..340
    atb,                 // Attribute byte
    shift_hi, shift_lo,  // Pattern table shift registers
    cycles,              // Cycle count for current instruction
    frame_buffer[61440]; // 256x240 pixel frame buffer. Top and bottom 8 rows
                         // are not drawn.

int shift_at = 0;

static int key_is_down(int virtual_key) {
#ifdef SMOLNES_HEADLESS_TRACE
  return virtual_key == VK_RETURN &&
         ((headless_frame >= 120 && headless_frame < 122) ||
          (headless_frame >= 180 && headless_frame < 182));
#else
  return GetForegroundWindow() == main_window &&
         (GetAsyncKeyState(virtual_key) & 0x8000) != 0;
#endif
}

// Prepare the current scanline's sprite list once, instead of searching all
// 64 OAM entries for every visible pixel. The first eight in-range sprites
// retain OAM priority, matching the NES per-scanline sprite limit.
static void prepare_scanline_sprites(void) {
  scanline_sprite_count = 0;
  scanline_sprite_height = (ppuctrl & 32) ? 16 : 8;
  for (uint16_t x = 0; x < 256; x++)
    scanline_sprite_pixel_owner[x] = 0;
  if (scany >= 240)
    return;

  for (uint8_t i = 0; i < 64 && scanline_sprite_count < 8; i++) {
    uint16_t row = (uint16_t)(scany - oam[i * 4] - 1);
    if (row < scanline_sprite_height) {
      scanline_sprite_indices[scanline_sprite_count] = i;
      scanline_sprite_rows[scanline_sprite_count] = (uint8_t)row;
      scanline_sprite_count++;
    }
  }

  // Decode the selected sprites once for this scanline and preserve the
  // first opaque OAM pixel at each screen position.
  for (uint8_t candidate = 0; candidate < scanline_sprite_count; candidate++) {
    uint8_t sprite_index = scanline_sprite_indices[candidate];
    uint8_t *sprite = oam + sprite_index * 4;
    uint16_t row = scanline_sprite_rows[candidate];
    uint16_t tile = sprite[1];
    uint16_t sy = row ^ (sprite[2] & 128 ? scanline_sprite_height - 1 : 0);
    uint16_t address =
        (ppuctrl & 32
             ? tile % 2 << 12 | tile << 4 & -32 | sy * 2 & 16
             : (ppuctrl & 8) << 9 | tile << 4) |
        sy & 7;
    uint8_t low = *get_chr_byte(address);
    uint8_t high = *get_chr_byte(address + 8);

    for (uint16_t x = 0; x < 8; x++) {
      uint16_t screen_x = sprite[3] + x;
      uint16_t bit = x ^ (!(sprite[2] & 64) * 7);
      uint8_t color = high >> bit << 1 & 2 | low >> bit & 1;
      if (screen_x < 256 && color && !scanline_sprite_pixel_owner[screen_x]) {
        scanline_sprite_pixel_color[screen_x] = color;
        scanline_sprite_pixel_attributes[screen_x] = sprite[2];
        scanline_sprite_pixel_owner[screen_x] = sprite_index + 1;
      }
    }
  }
}

static void paint_frame(HDC dc, const RECT *client) {
  int width = client->right - client->left;
  int height = client->bottom - client->top;
  if (width <= 0 || height <= 0)
    return;
  StretchDIBits(dc, 0, 0, width, height, 0, 0, 256, 224,
                frame_buffer + 8 * 256,
                (const BITMAPINFO *)&frame_bitmap_info, DIB_RGB_COLORS,
                SRCCOPY);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam,
                                    LPARAM lparam) {
  switch (message) {
  case WM_CLOSE:
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    app_running = 0;
    PostQuitMessage(0);
    return 0;
  case MM_WOM_DONE:
    audio_handle_done((void *)lparam);
    return 0;
  case WM_PAINT: {
    PAINTSTRUCT paint;
    HDC dc = BeginPaint(hwnd, &paint);
    RECT client;
    GetClientRect(hwnd, &client);
    paint_frame(dc, &client);
    EndPaint(hwnd, &paint);
    return 0;
  }
  case WM_ERASEBKGND:
    return 1;
  }
  return DefWindowProcA(hwnd, message, wparam, lparam);
}

static int create_native_window(void) {
  WNDCLASSEXA window_class = {0};
  window_class.cbSize = sizeof window_class;
  window_class.lpfnWndProc = window_proc;
  window_class.hInstance = GetModuleHandleA(NULL);
  window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
  window_class.lpszClassName = "smolnes-native-window";
  if (!RegisterClassExA(&window_class))
    return 0;

  RECT rect = {0, 0, 1024, 840};
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
  main_window = CreateWindowExA(
      0, window_class.lpszClassName, "smolnes", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left,
      rect.bottom - rect.top, NULL, NULL, window_class.hInstance, NULL);
  if (!main_window)
    return 0;
  ShowWindow(main_window, SW_SHOWDEFAULT);
  UpdateWindow(main_window);
  return 1;
}

static int pump_native_messages(void) {
  MSG message;
  while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
    if (message.message == WM_QUIT)
      return 0;
    TranslateMessage(&message);
    DispatchMessageA(&message);
  }
  return app_running;
}

#ifndef SMOLNES_OMIT_TIMER_FAILURE_CHECKS
#define SMOLNES_TIMER_CHECK(expression) \
  do {                                      \
    if (!(expression))                     \
      goto timer_setup_failed;             \
  } while (0)
#else
#define SMOLNES_TIMER_CHECK(expression) ((void)(expression))
#endif

static int initialize_frame_timers(void) {
#if defined(SMOLNES_FRAME_TIME_DIAGNOSTICS) || defined(SMOLNES_PACING_STRESS_TEST)
  if (!QueryPerformanceFrequency(&frame_frequency))
    return 0;
#endif
  LARGE_INTEGER first_16ms, first_50ms;
  first_16ms.QuadPart = -160000;
  first_50ms.QuadPart = -500000;
  frame_timer_16 = CreateWaitableTimerA(NULL, FALSE, NULL);
  SMOLNES_TIMER_CHECK(frame_timer_16 != NULL);
  frame_timer_50 = CreateWaitableTimerA(NULL, FALSE, NULL);
  SMOLNES_TIMER_CHECK(frame_timer_50 != NULL);
  SMOLNES_TIMER_CHECK(
      SetWaitableTimer(frame_timer_16, &first_16ms, 16, NULL, NULL, FALSE));
  SMOLNES_TIMER_CHECK(
      SetWaitableTimer(frame_timer_50, &first_50ms, 50, NULL, NULL, FALSE));
  return 1;
#ifndef SMOLNES_OMIT_TIMER_FAILURE_CHECKS
timer_setup_failed:
  if (frame_timer_16) {
    CancelWaitableTimer(frame_timer_16);
    CloseHandle(frame_timer_16);
    frame_timer_16 = NULL;
  }
  if (frame_timer_50) {
    CancelWaitableTimer(frame_timer_50);
    CloseHandle(frame_timer_50);
    frame_timer_50 = NULL;
  }
  return 0;
#endif
}

#undef SMOLNES_TIMER_CHECK
// Pace two frames at 16 ms, then synchronize the third to the 50 ms timer.
static int wait_for_next_frame(void) {
#ifdef SMOLNES_HEADLESS_TRACE
  uint64_t apu_cycle = apu_diagnostic_time();
  if (apu_cycle >= headless_audio_last_apu_cycle)
    headless_audio_cycle_remainder +=
        (apu_cycle - headless_audio_last_apu_cycle) * 48000u;
  headless_audio_last_apu_cycle = apu_cycle;
  while (headless_audio_cycle_remainder >= 1786840u * 480u) {
    short pcm[480 * 2];
    short dmc_pcm[480 * 2];
    long long energy = 0;
    int peak = 0;
    unsigned i;
    audio_test_render_with_dmc(pcm, dmc_pcm, 480);
    if (headless_mix_pcm)
      fwrite(pcm, sizeof(pcm[0]), 960, headless_mix_pcm);
    if (headless_dmc_pcm)
      fwrite(dmc_pcm, sizeof(dmc_pcm[0]), 960, headless_dmc_pcm);
    for (i = 0; i < 960; i++) {
      int value = pcm[i];
      int magnitude = value < 0 ? -value : value;
      if (magnitude > peak) peak = magnitude;
      energy += (long long)value * value;
    }
    if ((unsigned)peak > headless_frame_audio_peak)
      headless_frame_audio_peak = (unsigned)peak;
    headless_frame_audio_energy += energy;
    headless_audio_cycle_remainder -= 1786840u * 480u;
  }
  if (!headless_apu_time_log) {
    headless_apu_time_log = fopen("headless-apu-time.csv", "w");
    if (headless_apu_time_log)
      fputs("frame,apu_cycle\n", headless_apu_time_log);
  }
  if (headless_apu_time_log)
    fprintf(headless_apu_time_log, "%u,%llu\n", headless_frame,
            (unsigned long long)apu_cycle);
  headless_frame++;
  if (headless_pcm_log && (headless_frame % 30 == 0)) {
    fprintf(headless_pcm_log, "%u,%u,%lld,%d\n", headless_frame,
            headless_frame_audio_peak, headless_frame_audio_energy,
            audio_test_dmc_peak());
    fflush(headless_pcm_log);
    headless_frame_audio_peak = 0;
    headless_frame_audio_energy = 0;
  }
  return headless_frame < 1200;
#else
  LARGE_INTEGER first_16ms;
  HANDLE timer;
  DWORD result;

  if (++frame_wait_count == 3) {
    frame_wait_count = 0;
    timer = frame_timer_50;
  } else {
    timer = frame_timer_16;
  }
  for (;;) {
    result = MsgWaitForMultipleObjects(1, &timer, FALSE, INFINITE, QS_ALLINPUT);
    if (result == WAIT_OBJECT_0) {
      if (frame_wait_count == 0) {
        // Re-anchor the 16 ms cadence at each 50 ms boundary.
        first_16ms.QuadPart = -160000;
        if (!SetWaitableTimer(frame_timer_16, &first_16ms, 16,
                              NULL, NULL, FALSE))
          return 0;
      }
      return app_running;
    }
    if (result == WAIT_OBJECT_0 + 1) {
      if (!pump_native_messages())
        return 0;
      continue;
    }
    return 0;
  }
#endif
}
// Read a byte from CHR ROM or CHR RAM.
uint8_t *get_chr_byte(uint16_t a) {
  return &chrrom[chr[a >> chrbits] << chrbits | a % (1 << chrbits)];
}

// Read a byte from nametable RAM.
uint8_t *get_nametable_byte(uint16_t a) {
  return &vram[mirror == 0   ? a % 1024                  // single bank 0
               : mirror == 1 ? a % 1024 + 1024           // single bank 1
               : mirror == 2 ? a & 2047                  // vertical mirroring
                             : a / 2 & 1024 | a % 1024]; // horizontal mirroring
}

// If `write` is non-zero, writes `val` to the address `hi:lo`, otherwise reads
// a value from the address `hi:lo`.
uint8_t mem(uint8_t lo, uint8_t hi, uint8_t val, uint8_t write) {
  uint16_t addr = hi << 8 | lo;

  switch (hi >>= 4) {
  case 0: case 1: // $0000...$1fff RAM
    return write ? ram[addr] = val : ram[addr];

  case 2: case 3: // $2000..$2007 PPU (mirrored)
    lo &= 7;

    // read/write $2007
    if (lo == 7) {
      tmp = ppubuf;
      uint8_t *rom =
          // Access CHR ROM or CHR RAM
          V < 8192 ? write && chrrom != chrram ? &tmp : get_chr_byte(V)
          // Access nametable RAM
          : V < 16128 ? get_nametable_byte(V)
                      // Access palette RAM
                      : palette_ram + (uint8_t)((V & 19) == 16 ? V ^ 16 : V);
      write ? *rom = val : (ppubuf = *rom);
      V += ppuctrl & 4 ? 32 : 1;
      V %= 16384;
      return tmp;
    }

    if (write)
      switch (lo) {
      case 0: // $2000 ppuctrl
        ppuctrl = val;
        T = T & 0xf3ff | val % 4 << 10;
        break;

      case 1: // $2001 ppumask
        ppumask = val;
        break;

      case 5: // $2005 ppuscroll
        T = (W ^= 1)
          ? fine_x = val & 7, T & ~31 | val / 8
          : T & 0x8c1f | val % 8 << 12 | val * 4 & 0x3e0;
        break;

      case 6: // $2006 ppuaddr
        T = (W ^= 1)
          ? T & 0xff | val % 64 << 8
          : (V = T & ~0xff | val);
      }

    if (lo == 2) { // $2002 ppustatus
      tmp = ppustatus & 0xe0;
      ppustatus &= 0x7f;
      W = 0;
      return tmp;
    }
    break;

  case 4:
    if (addr >= 0x4000 && addr <= 0x4013 && write) {
      apu_write(addr, val);
      return 0;
    }
    if (addr == 0x4015) {
      if (write)
        apu_write(addr, val);
      else
        return apu_read_status();
      return 0;
    }
    if (addr == 0x4017 && write) {
      apu_write(addr, val);
      return 0;
    }
    if (write && lo == 20) // $4014 OAM DMA
      for (uint16_t i = 256; i--;)
        oam[i] = mem(i, val, 0, 0);
    // $4016 Joypad 1
    for (tmp = 0, hi = 8; hi--;)
      tmp = tmp * 2 + key_is_down((uint8_t[]){
                                      'X',      // A
                                      'Z',      // B
                                      VK_TAB,   // Select
                                      VK_RETURN, // Start
                                      VK_UP,    // Dpad Up
                                      VK_DOWN,  // Dpad Down
                                      VK_LEFT,  // Dpad Left
                                      VK_RIGHT, // Dpad Right
                                  }[hi]);
    if (lo == 22) {
      if (write) {
        keys = tmp;
      } else {
        tmp = keys & 1;
        keys /= 2;
        return tmp;
      }
    }
    return 0;

  case 6: case 7: // $6000...$7fff PRG RAM
    addr &= 8191;
    return write ? prgram[addr] = val : prgram[addr];

  default: // $8000...$ffff ROM
    // handle mapper writes
    if (write)
      switch (rom_header[6] >> 4) {
      case 7: // mapper 7
        mirror = !(val / 16);
        prg[0] = val % 8 * 2;
        prg[1] = prg[0] + 1;
        break;

      case 4: { // mapper 4
        uint8_t addr1 = addr & 1;
        switch (hi >> 1) {
          case 4: // Bank select/bank data
            *(addr1 ? &mmc3_chrprg[mmc3_bits & 7] : &mmc3_bits) = val;
            tmp = mmc3_bits >> 5 & 4;
            for (int i = 4; i--;) {
              chr[0 + i + tmp] = mmc3_chrprg[i / 2] & ~!(i % 2) | i % 2;
              chr[4 + i - tmp] = mmc3_chrprg[2 + i];
            }
            tmp = mmc3_bits >> 5 & 2;
            prg[0 + tmp] = mmc3_chrprg[6];
            prg[1] = mmc3_chrprg[7];
            prg[3] = rom_header[4] * 2 - 1;
            prg[2 - tmp] = prg[3] - 1;
            break;
          case 5: // Mirroring
            if (!addr1) {
              mirror = 2 + val % 2;
            }
            break;
          case 6:  // IRQ Latch
            if (!addr1) {
              mmc3_latch = val;
            }
            break;
          case 7:  // IRQ Enable
            mmc3_irq = addr1;
            break;
        }
        break;
      }

      case 3: // mapper 3
        chr[0] = val % 4 * 2;
        chr[1] = chr[0] + 1;
        break;

      case 2: // mapper 2
        prg[0] = val & 31;
        break;

      case 1: // mapper 1
        if (val & 0x80) {
          mmc1_bits = 5;
          mmc1_data = 0;
          mmc1_ctrl |= 12;
        } else if (mmc1_data = mmc1_data / 2 | val << 4 & 16, !--mmc1_bits) {
          mmc1_bits = 5;
          tmp = addr >> 13;
          *(tmp == 4 ? mirror = mmc1_data & 3, &mmc1_ctrl
          : tmp == 5 ? &chrbank0
          : tmp == 6 ? &chrbank1
                     : &prgbank) = mmc1_data;

          // Update CHR banks.
          chr[0] = chrbank0 & ~!(mmc1_ctrl & 16);
          chr[1] = mmc1_ctrl & 16 ? chrbank1 : chrbank0 | 1;

          // Update PRG banks.
          tmp = mmc1_ctrl / 4 % 4 - 2;
          prg[0] = !tmp ? 0 : tmp == 1 ? prgbank : prgbank & ~1;
          prg[1] = !tmp ? prgbank : tmp == 1 ? rom_header[4] - 1 : prgbank | 1;
        }
      }
    return rom[(prg[hi - 8 >> prgbits - 12] & (rom_header[4] << 14 - prgbits) - 1)
                   << prgbits |
               addr & (1 << prgbits) - 1];
  }

  return ~0;
}

static uint8_t apu_read_dmc_memory(uint16_t address) {
  return mem((uint8_t)address, (uint8_t)(address >> 8), 0, 0);
}

// Read a byte at address `pc_hi:PCL` and increment PC.
uint8_t read_pc() {
  val = mem(PCL, pc_hi, 0, 0);
  !++PCL && ++pc_hi;
  return val;
}

// Set N (negative) and Z (zero) flags of `P` register, based on `val`.
uint8_t set_nz(uint8_t val) { return P = P & 125 | val & 128 | !val * 2; }

static char rom_path[MAX_PATH];
static OPENFILENAMEA rom_dialog;

static char *select_rom_file(void) {
  char *p = GetCommandLineA();
  if (*p == '"') {
    ++p;
    while (*p && *p != '"') ++p;
    if (*p) ++p;
  } else {
    while (*p && *p != ' ') ++p;
  }
  while (*p == ' ') ++p;
  if (*p)
    return p;
  rom_dialog.lStructSize = sizeof rom_dialog;
  rom_dialog.lpstrFile = rom_path;
  rom_dialog.nMaxFile = sizeof rom_path;
#ifdef SMOLNES_ENABLE_7Z_LZMA1
  rom_dialog.lpstrFilter =
      "NES ROMs (*.nes;*.7z)\0*.nes;*.7z\0All files (*.*)\0*.*\0\0";
#else
  rom_dialog.lpstrFilter =
      "NES ROMs (*.nes)\0*.nes\0All files (*.*)\0*.*\0\0";
#endif
  rom_dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
  return GetOpenFileNameA(&rom_dialog) ? rom_path : NULL;
}

#if defined(SMOLNES_ENABLE_7Z_LZMA1) && defined(SMOLNES_7Z_DEBUG)
/* Read the same 7z integer subset as the fast library, while proving that its
   four-byte speculative reads and the consumed value stay inside the header. */
static int read_7z_number_checked(const uint8_t *archive, size_t archive_size,
                                 size_t header_end, size_t *position,
                                 uint32_t *value) {
  uint32_t first, raw;
  unsigned length;
  if (*position > archive_size || archive_size - *position < 4 ||
      *position > header_end)
    return 0;
  raw = (uint32_t)archive[*position] |
        (uint32_t)archive[*position + 1] << 8 |
        (uint32_t)archive[*position + 2] << 16 |
        (uint32_t)archive[*position + 3] << 24;
  first = raw & 0xffu;
  if (!(first & 0x80u)) {
    length = 1;
    *value = first;
  } else if (!(first & 0x40u)) {
    length = 2;
    *value = ((raw >> 8) & 0xffu) | ((first & 0x3fu) << 8);
  } else if (!(first & 0x20u)) {
    length = 3;
    *value = ((raw >> 8) & 0xffffu) | ((first & 0x1fu) << 16);
  } else {
    length = 4;
    *value = ((raw >> 8) & 0xffffffu) | ((first & 0x0fu) << 24);
  }
  if (length > header_end - *position)
    return 0;
  *position += length;
  return 1;
}

/* Preflight exactly the single-folder LZMA1 layout accepted by the library.
   The release .lib trusts its caller and does not enforce output_capacity. */
static int inspect_single_lzma1_7z(const uint8_t *archive,
                                   size_t archive_size,
                                   size_t *unpacked_size) {
  static const uint8_t signature[6] = {0x37, 0x7a, 0xbc, 0xaf, 0x27, 0x1c};
  uint64_t next_offset, next_size;
  size_t header_start, header_end, position, packed_offset;
  uint32_t number, packed_size, folder_count, coder_count;
  uint8_t tag, all_digests;

  if (archive_size < 32 || archive_size >= 100u * 1024u * 1024u ||
      archive[0] != signature[0] || archive[1] != signature[1] ||
      archive[2] != signature[2] || archive[3] != signature[3] ||
      archive[4] != signature[4] || archive[5] != signature[5])
    return 0;
  next_offset = (uint64_t)archive[12] | (uint64_t)archive[13] << 8 |
                (uint64_t)archive[14] << 16 | (uint64_t)archive[15] << 24 |
                (uint64_t)archive[16] << 32 | (uint64_t)archive[17] << 40 |
                (uint64_t)archive[18] << 48 | (uint64_t)archive[19] << 56;
  next_size = (uint64_t)archive[20] | (uint64_t)archive[21] << 8 |
              (uint64_t)archive[22] << 16 | (uint64_t)archive[23] << 24 |
              (uint64_t)archive[24] << 32 | (uint64_t)archive[25] << 40 |
              (uint64_t)archive[26] << 48 | (uint64_t)archive[27] << 56;
  if (next_offset > archive_size - 32)
    return 0;
  header_start = 32 + (size_t)next_offset;
  if (next_size > archive_size - header_start)
    return 0;
  header_end = header_start + (size_t)next_size;
  position = header_start;

  /* Header -> MainStreamsInfo -> PackInfo; one stream with a packed size. */
  if (header_end - position < 3 || archive[position++] != 0x01 ||
      archive[position++] != 0x04 || archive[position++] != 0x06 ||
      !read_7z_number_checked(archive, archive_size, header_end, &position,
                              &number) ||
      number >= 100u * 1024u * 1024u || header_end - position < 2 ||
      archive[position++] != 1 || archive[position++] != 9 ||
      !read_7z_number_checked(archive, archive_size, header_end, &position,
                              &packed_size))
    return 0;
  if (position >= header_end)
    return 0;
  tag = archive[position++];
  if (tag == 10) {
    if (position >= header_end)
      return 0;
    all_digests = archive[position++];
    if (all_digests) {
      if (header_end - position < 4)
        return 0;
      position += 4;
    } else {
      if (position >= header_end)
        return 0;
      if (archive[position++] & 0x80u) {
        if (header_end - position < 4)
          return 0;
        position += 4;
      }
    }
    if (position >= header_end || archive[position++] != 0)
      return 0;
  } else if (tag != 0) {
    return 0;
  }
  packed_offset = 32 + (size_t)number;
  if (!packed_size || packed_offset > header_start ||
      packed_size > header_start - packed_offset)
    return 0;

  /* The remaining fixed fields must match the fast decoder's assumptions. */
  if (header_end - position < 2 || archive[position++] != 7 ||
      archive[position++] != 0x0b ||
      !read_7z_number_checked(archive, archive_size, header_end, &position,
                              &folder_count) ||
      folder_count != 1 || position >= header_end || archive[position++] != 0 ||
      !read_7z_number_checked(archive, archive_size, header_end, &position,
                              &coder_count) ||
      coder_count != 1 || header_end - position < 9 ||
      archive[position++] != 0x23 || archive[position++] != 0x03 ||
      archive[position++] != 0x01 || archive[position++] != 0x01 ||
      archive[position++] != 5)
    return 0;
  if (header_end - position < 6 || archive[position] >= 225)
    return 0;
  position += 5;
  if (archive[position++] != 0x0c ||
      !read_7z_number_checked(archive, archive_size, header_end, &position,
                              &number) ||
      number < 16 || number > ROM_BUFFER_SIZE)
    return 0;
  *unpacked_size = number;
  return 1;
}
#endif

static int run_emulator(const char *rom_path) {
#ifdef SMOLNES_ENABLE_7Z_LZMA1
  int rom_is_7z = 0;
#endif
  HANDLE rom_file = CreateFileA(rom_path, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (rom_file == INVALID_HANDLE_VALUE) {
#ifdef SMOLNES_7Z_VALIDATE_ONLY
    return 1;
#else
    MessageBoxA(NULL, rom_path, "Could not open ROM",
                MB_OK | MB_ICONERROR);
    return 1;
#endif
  }
  DWORD rom_bytes = 0;
#ifdef SMOLNES_ENABLE_7Z_LZMA1
  uint8_t *rom_input = rom_archive;
  BOOL rom_read_ok = ReadFile(rom_file, rom_input, (DWORD)ROM_BUFFER_SIZE,
                              &rom_bytes, NULL);
#else
  BOOL rom_read_ok = ReadFile(rom_file, rombuf, (DWORD)sizeof rombuf,
                              &rom_bytes, NULL);
#endif
  BOOL rom_close_ok = CloseHandle(rom_file);
  if (!rom_read_ok || !rom_close_ok) {
#ifdef SMOLNES_7Z_VALIDATE_ONLY
    return 2;
#else
    MessageBoxA(NULL, rom_path, "Could not read ROM", MB_OK | MB_ICONERROR);
    return 1;
#endif
  }
#ifdef SMOLNES_ENABLE_7Z_LZMA1
  if (rom_bytes >= 6 && rom_input[0] == 0x37 && rom_input[1] == 0x7a &&
      rom_input[2] == 0xbc && rom_input[3] == 0xaf && rom_input[4] == 0x27 &&
      rom_input[5] == 0x1c) {
    rom_is_7z = 1;
    size_t decoded_size = 0;
    SevenZipResult decode_result;
#ifdef SMOLNES_7Z_DEBUG
    size_t expected_size = 0;
    if (!inspect_single_lzma1_7z(rom_archive, rom_bytes, &expected_size)) {
#ifdef SMOLNES_7Z_VALIDATE_ONLY
      return 3;
#else
      MessageBoxA(NULL, "This 7z file is not a supported single-file LZMA1 ROM",
                  "Could not load ROM", MB_OK | MB_ICONERROR);
      return 1;
#endif
    }
#endif
    decode_result = sevenzip_extract_single_lzma1_fast(
        rom_archive, rom_bytes, rombuf, sizeof rombuf, &decoded_size);
    if (decode_result != SEVENZIP_OK) {
#ifdef SMOLNES_7Z_VALIDATE_ONLY
      return 4;
#else
      MessageBoxA(NULL, "Could not decode a valid NES ROM from this 7z file",
                  "Could not load ROM", MB_OK | MB_ICONERROR);
      return 1;
#endif
    }
#ifdef SMOLNES_7Z_DEBUG
    if (decoded_size != expected_size || rombuf[0] != 'N' ||
        rombuf[1] != 'E' || rombuf[2] != 'S' || rombuf[3] != 0x1a) {
#ifdef SMOLNES_7Z_VALIDATE_ONLY
      return 5;
#else
      MessageBoxA(NULL, "Decoded data does not contain a valid NES ROM",
                  "Could not load ROM", MB_OK | MB_ICONERROR);
      return 1;
#endif
    }
#endif
    rom_bytes = (DWORD)decoded_size;
    rom_header = rombuf;
  } else {
    rom_header = rom_input;
  }
#endif
#ifdef SMOLNES_ENABLE_7Z_LZMA1
  if (rom_bytes < 16 && !rom_is_7z) {
#else
  if (rom_bytes < 16) {
#endif
#ifdef SMOLNES_7Z_VALIDATE_ONLY
    return 6;
#else
    MessageBoxA(NULL, rom_path, "ROM is too small to contain an iNES header",
                MB_OK | MB_ICONERROR);
    return 1;
#endif
  }
  // Start PRG0 after 16-byte header.
  rom = rom_header + 16;
  // PRG1 is the last bank. `rom_header[4]` is the number of 16k PRG banks.
  prg[1] = rom_header[4] - 1;
  // CHR0 ROM is after all PRG data in the file. `rom_header[5]` is the number of
  // 8k CHR banks. If it is zero, assume the game uses CHR RAM.
  chrrom = rom_header[5] ? rom + (rom_header[4] << 14) : chrram;
  // CHR1 is the last 4k bank.
  chr[1] = rom_header[5] ? rom_header[5] * 2 - 1 : 1;
  // Bit 0 of `rom_header[6]` is 0=>horizontal mirroring, 1=>vertical mirroring.
  mirror = 3 - rom_header[6] % 2;
  if (rom_header[6] / 16 == 4) {
    mem(0, 128, 0, 1); // Update to default mmc3 banks
    prgbits--;         // 8kb PRG banks
    chrbits -= 2;      // 1kb CHR banks
  }

#ifdef SMOLNES_7Z_VALIDATE_ONLY
  return 0;
#endif

  // Start at address in reset vector, at $FFFC.
  PCL = mem(~3, ~0, 0, 0);
  pc_hi = mem(~2, ~0, 0, 0);

#ifndef SMOLNES_HEADLESS_TRACE
  if (!create_native_window()) {
    MessageBoxA(NULL, "Could not create the emulator window", "smolnes",
                MB_OK | MB_ICONERROR);
    return 1;
  }
  if (!initialize_frame_timers()) {
    MessageBoxA(NULL, "Could not initialize the frame timers", "smolnes",
                MB_OK | MB_ICONERROR);
    DestroyWindow(main_window);
    return 1;
  }
#else
  audio_test_reset();
  {
    short prefill[480 * 2];
    unsigned i;
    for (i = 0; i < 6; i++) audio_test_render(prefill, 480);
  }
  headless_pcm_log = fopen("headless-pcm.csv", "w");
  headless_mix_pcm = fopen("headless-mix.pcm", "wb");
  headless_dmc_pcm = fopen("headless-dmc.pcm", "wb");
  if (headless_pcm_log)
    fputs("frame,peak,energy,dmc_peak\n", headless_pcm_log);
#endif
  apu_set_memory_reader(apu_read_dmc_memory);
#ifndef SMOLNES_HEADLESS_TRACE
  if (!audio_start(main_window))
    MessageBoxA(main_window, "Audio output is unavailable; running muted.",
                "smolnes", MB_OK | MB_ICONWARNING);
#endif
  apu_reset();
#ifdef SMOLNES_HEADLESS_TRACE
  headless_audio_last_apu_cycle = apu_diagnostic_time();
  headless_audio_cycle_remainder = 0;
#endif
#ifdef SMOLNES_FRAME_TIME_DIAGNOSTICS
  frame_timing_log = fopen("frame-time.csv", "w");
  if (frame_timing_log)
    fputs("sample_frames,avg_work_ms,min_work_ms,max_work_ms,avg_gdi_ms,min_gdi_ms,max_gdi_ms\n",
          frame_timing_log);
#endif

loop:
  cycles = nomem = 0;
  if (nmi_irq)
    goto nmi_irq;

  opcode = read_pc();
  uint8_t opcodelo5 = opcode & 31;
  switch (opcodelo5) {
  case 0:
    if (opcode & 0x80) { // LDY/CPY/CPX imm
      read_pc();
      nomem = 1;
      goto nomemop;
    }

    switch (opcode >> 5) {
    case 0: { // BRK or nmi_irq
      !++PCL && ++pc_hi;
    nmi_irq:
      PUSH(pc_hi);
      PUSH(PCL);
      PUSH(P | 32);
      // BRK/IRQ vector is $ffff, NMI vector is $fffa
      uint16_t veclo = ~1 - (nmi_irq & 4);
      PCL = mem(veclo, ~0, 0, 0);
      pc_hi = mem(veclo + 1, ~0, 0, 0);
      nmi_irq = 0;
      cycles++;
      break;
    }

    case 1: // JSR
      result = read_pc();
      PUSH(pc_hi);
      PUSH(PCL);
      pc_hi = read_pc();
      PCL = result;
      break;

    case 2: // RTI
      P = PULL & ~32;
      PCL = PULL;
      pc_hi = PULL;
      break;

    case 3: // RTS
      PCL = PULL;
      pc_hi = PULL;
      !++PCL && ++pc_hi;
      break;
    }

    cycles += 4;
    break;

  case 16: // BPL, BMI, BVC, BVS, BCC, BCS, BNE, BEQ
    read_pc();
    if (!(P & mask[opcode >> 6]) ^ opcode / 32 & 1) {
      cross = PCL + (int8_t)val >> 8;
      pc_hi += cross;
      PCL += val;
      cycles += cross ? 2 : 1;
    }
    break;

  case 8: case 24:
    switch (opcode >>= 4) {
    case 0: // PHP
      PUSH(P | 48);
      cycles++;
      break;

    case 2: // PLP
      P = PULL & ~16;
      cycles += 2;
      break;

    case 4: // PHA
      PUSH(A);
      cycles++;
      break;

    case 6: // PLA
      set_nz(A = PULL);
      cycles += 2;
      break;

    case 8: // DEY
      set_nz(--Y);
      break;

    case 9: // TYA
      set_nz(A = Y);
      break;

    case 10: // TAY
      set_nz(Y = A);
      break;

    case 12: // INY
      set_nz(++Y);
      break;

    case 14: // INX
      set_nz(++X);
      break;

    default: // CLC, SEC, CLI, SEI, CLV, CLD, SED
      P = P & ~mask[opcode + 3] | mask[opcode + 4];
      break;
    }
    break;

  case 10: case 26:
    switch (opcode >> 4) {
    case 8: // TXA
      set_nz(A = X);
      break;

    case 9: // TXS
      S = X;
      break;

    case 10: // TAX
      set_nz(X = A);
      break;

    case 11: // TSX
      set_nz(X = S);
      break;

    case 12: // DEX
      set_nz(--X);
      break;

    case 14: // NOP
      break;

    default: // ASL/ROL/LSR/ROR A
      nomem = 1;
      val = A;
      goto nomemop;
    }
    break;

  case 1: // X-indexed, indirect
    read_pc();
    val += X;
    addr_lo = mem(val, 0, 0, 0);
    addr_hi = mem(val + 1, 0, 0, 0);
    cycles += 4;
    goto opcode;

  case 2: case 9: // Immediate
    read_pc();
    nomem = 1;
    goto nomemop;

  case 17: // Zeropage, Y-indexed
    addr_lo = mem(read_pc(), 0, 0, 0);
    addr_hi = mem(val + 1, 0, 0, 0);
    cycles++;
    goto add_x_or_y;

  case 4: case 5: case 6:     // Zeropage               +1
  case 20: case 21: case 22:  // Zeropage, X-indexed    +2
    addr_lo = read_pc();
    cross = opcodelo5 > 6;
    if (cross) {
      addr_lo += (opcode & 214) == 150 ? Y : X;  // LDX/STX use Y
    }
    addr_hi = 0;
    cycles -= !cross;
    goto opcode;

  case 12: case 13: case 14: // Absolute               +2
  case 25:                   // Absolute, Y-indexed    +2/3
  case 28: case 29: case 30: // Absolute, X-indexed    +2/3
    addr_lo = read_pc();
    addr_hi = read_pc();
    if (opcodelo5 < 25) goto opcode;
  add_x_or_y:
    val = opcodelo5 < 28 | opcode == 190 ? Y : X;
    cross = addr_lo + val > 255;
    addr_hi += cross;
    addr_lo += val;
    cycles +=
        ((opcode & 224) == 128 | opcode % 16 == 14 & opcode != 190) | cross;
  opcode:
    cycles += 2;
    if (opcode != 76 & (opcode & 224) != 128) {
      val = mem(addr_lo, addr_hi, 0, 0);
    }

  nomemop:
    result = 0;
    switch (opcode & 227) {
    case 1: set_nz(A |= val); break;  // ORA
    case 33: set_nz(A &= val); break; // AND
    case 65: set_nz(A ^= val); break; // EOR
    case 225: // SBC
      val = ~val;
      // fallthrough
    case 97: // ADC
      sum = A + val + P % 2;
      P = P & ~65 | sum > 255 | ((A ^ sum) & (val ^ sum) & 128) / 2;
      set_nz(A = sum);
      break;

    case 34: // ROL
      result = P & 1;
      // fallthrough
    case 2: // ASL
      result |= val * 2;
      P = P & ~1 | val / 128;
      goto memop;

    case 98: // ROR
      result = P << 7;
      // fallthrough
    case 66: // LSR
      result |= val / 2;
      P = P & ~1 | val & 1;
      goto memop;

    case 194: // DEC
      result = val - 1;
      goto memop;

    case 226: // INC
      result = val + 1;
      // fallthrough

    memop:
      set_nz(result);
      // Write result to A or back to memory.
      nomem ? A = result : (cycles += 2, mem(addr_lo, addr_hi, result, 1));
      break;

    case 32: // BIT
      P = P & 61 | val & 192 | !(A & val) * 2;
      break;

    case 64: // JMP
      PCL = addr_lo;
      pc_hi = addr_hi;
      cycles--;
      break;

    case 96: // JMP indirect
      PCL = val;
      pc_hi = mem(addr_lo + 1, addr_hi, 0, 0);
      cycles++;
      break;

    default: {
      uint8_t opcodehi3 = opcode / 32;
      uint8_t *reg = opcode % 4 == 2 | opcodehi3 == 7 ? &X
                     : opcode % 4 == 1                ? &A
                                                      : &Y;
      if (opcodehi3 == 4) {  // STY/STA/STX
        mem(addr_lo, addr_hi, *reg, 1);
      } else if (opcodehi3 != 5) {  // CPY/CMP/CPX
        P = P & ~1 | *reg >= val;
        set_nz(*reg - val);
      } else {  // LDY/LDA/LDX
        set_nz(*reg = val);
      }
      break;
    }
    }
  }

  // DMC sample fetches stall CPU and PPU for four cycles per byte.
  /* `cycles` stores cycles beyond the two-cycle instruction minimum.
     The PPU path adds those two base cycles as `+6` dots below; the APU must
     receive them as well or its clock runs about three times too slowly. */
  cycles += apu_clock(cycles + 2);
  /* The APU frame IRQ is maskable by the CPU interrupt-disable flag. */
  if (!nmi_irq && !(P & 4) && apu_irq_pending())
    nmi_irq = 1;

  // Update PPU, which runs 3 times faster than CPU. Each CPU instruction
  // takes at least 2 cycles.
  for (tmp = cycles * 3 + 6; tmp--;) {
    if (dot == 0 && scany < 240)
      prepare_scanline_sprites();

    if (ppumask & 24) { // If background or sprites are enabled.
      if (scany < 240) {
        if (dot - 256 > 63u) {  // dot [0..255,320..340]
          // Draw a pixel to the framebuffer.
          if (dot < 256) {
            // Sample background color from the existing dot pipeline.
            scanline_background_color[dot] =
                shift_hi >> 14 - fine_x & 2 | shift_lo >> 15 - fine_x & 1;
            scanline_background_palette[dot] =
                shift_at >> 28 - fine_x * 2 & 12;
            uint8_t color = scanline_background_color[dot],
                    palette = scanline_background_palette[dot];

            // Composite the predecoded first opaque sprite pixel, if enabled.
            if (ppumask & 16) {
              uint8_t owner = scanline_sprite_pixel_owner[dot];
              if (owner) {
                uint8_t sprite_color = scanline_sprite_pixel_color[dot];
                uint8_t attributes = scanline_sprite_pixel_attributes[dot];
                if (!(attributes & 32 && color)) {
                  color = sprite_color;
                  palette = 16 | attributes * 4 & 12;
                }
                if (owner == 1 && color)
                  ppustatus |= 64;
              }
            }

            frame_buffer[scany * 256 + dot] =
                nes_palette_rgb565[palette_ram[color ? palette | color : 0]];
          }

          // Update shift registers every cycle.
          if (dot < 336) {
            shift_hi *= 2;
            shift_lo *= 2;
            shift_at *= 4;
          }

          int temp = ppuctrl << 8 & 4096 | ntb << 4 | V >> 12;
          switch (dot & 7) {
          case 1: // Read nametable byte.
            ntb = *get_nametable_byte(V);
            break;
          case 3: // Read attribute byte.
            atb = (*get_nametable_byte(V & 0xc00 | 0x3c0 | V >> 4 & 0x38 |
                                       V / 4 & 7) >>
                   (V >> 5 & 2 | V / 2 & 1) * 2) %
                  4 * 0x5555;
            break;
          case 5: // Read pattern table low byte.
            ptb_lo = *get_chr_byte(temp);
            break;
          case 7: { // Read pattern table high byte.
            uint8_t ptb_hi = *get_chr_byte(temp | 8);
            // Increment horizontal VRAM read address.
            V = V % 32 == 31 ? V & ~31 ^ 1024 : V + 1;
            shift_hi |= ptb_hi;
            shift_lo |= ptb_lo;
            shift_at |= atb;
            break;
          }
          }
        }

        // Increment vertical VRAM address.
        if (dot == 256) {
          V = ((V & 7 << 12) != 7 << 12 ? V + 4096
               : (V & 0x3e0) == 928     ? V & 0x8c1f ^ 2048
               : (V & 0x3e0) == 0x3e0   ? V & 0x8c1f
                                        : V & 0x8c1f | V + 32 & 0x3e0) &
                  // Reset horizontal VRAM address to T value.
                  ~0x41f |
              T & 0x41f;
        }
      }

      // Check for MMC3 IRQ.
      if ((scany + 1) % 262 < 241 && dot == 261 && mmc3_irq && !mmc3_latch--)
        nmi_irq = 1;

      // Reset vertical VRAM address to T value.
      if (scany == 261 && dot - 280 < 25u)  // dot [280..304]
        V = V & 0x841f | T & 0x7be0;
    }

    if (dot == 1) {
      if (scany == 241) {
        // If NMI is enabled, trigger NMI.
        if (ppuctrl & 128)
          nmi_irq = 4;
        ppustatus |= 128;
        // Render frame, skipping the top and bottom 8 pixels (they're often
        // garbage).
#ifndef SMOLNES_HEADLESS_TRACE
        InvalidateRect(main_window, NULL, FALSE);
#ifdef SMOLNES_FRAME_TIME_DIAGNOSTICS
        {
          LARGE_INTEGER paint_start, paint_end;
          QueryPerformanceCounter(&paint_start);
          UpdateWindow(main_window);
          QueryPerformanceCounter(&paint_end);
          LONGLONG paint_ticks = paint_end.QuadPart - paint_start.QuadPart;
          frame_paint_sum += paint_ticks;
          if (!frame_work_samples || paint_ticks < frame_paint_min)
            frame_paint_min = paint_ticks;
          if (paint_ticks > frame_paint_max)
            frame_paint_max = paint_ticks;
        }
#else
        UpdateWindow(main_window);
#endif
#endif
#ifdef SMOLNES_PACING_STRESS_TEST
        {
          LARGE_INTEGER stress_start, stress_now;
          QueryPerformanceCounter(&stress_start);
          do {
            QueryPerformanceCounter(&stress_now);
          } while ((stress_now.QuadPart - stress_start.QuadPart) * 1000 <
                   frame_frequency.QuadPart * 8);
        }
#endif
#ifdef SMOLNES_FRAME_TIME_DIAGNOSTICS
        {
          LARGE_INTEGER work_end;
          if (frame_work_started && QueryPerformanceCounter(&work_end)) {
            LONGLONG elapsed = work_end.QuadPart - frame_work_start.QuadPart;
            frame_work_sum += elapsed;
            if (!frame_work_samples || elapsed < frame_work_min)
              frame_work_min = elapsed;
            if (elapsed > frame_work_max)
              frame_work_max = elapsed;
            frame_work_samples++;
            if (frame_work_samples >= 120 && frame_timing_log) {
              fprintf(frame_timing_log,
                      "%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n",
                      frame_work_samples,
                      1000.0 * frame_work_sum / frame_work_samples /
                          frame_frequency.QuadPart,
                      1000.0 * frame_work_min / frame_frequency.QuadPart,
                      1000.0 * frame_work_max / frame_frequency.QuadPart,
                      1000.0 * frame_paint_sum / frame_work_samples /
                          frame_frequency.QuadPart,
                      1000.0 * frame_paint_min / frame_frequency.QuadPart,
                      1000.0 * frame_paint_max / frame_frequency.QuadPart);
              fflush(frame_timing_log);
              frame_work_sum = 0;
              frame_work_samples = 0;
              frame_work_min = frame_work_max = 0;
              frame_paint_sum = frame_paint_min = frame_paint_max = 0;
            }
          }
        }
#endif
#ifdef SMOLNES_FPS_DIAGNOSTICS
        {
          DWORD now = GetTickCount();
          if (!fps_sample_start)
            fps_sample_start = now;
          fps_sample_frames++;
          if ((DWORD)(now - fps_sample_start) >= 1000) {
            char title[64];
            DWORD elapsed = now - fps_sample_start;
            unsigned fps = (fps_sample_frames * 1000 + elapsed / 2) / elapsed;
            snprintf(title, sizeof title, "smolnes - %u FPS", fps);
            SetWindowTextA(main_window, title);
            fps_sample_start = now;
            fps_sample_frames = 0;
          }
        }
#endif
#ifndef SMOLNES_HEADLESS_TRACE
        if (!pump_native_messages())
          goto shutdown;
#endif
        if (!wait_for_next_frame())
          goto shutdown;
#ifdef SMOLNES_FRAME_TIME_DIAGNOSTICS
        frame_work_started = QueryPerformanceCounter(&frame_work_start);
#endif
      }

      // Clear ppustatus.
      if (scany == 261)
        ppustatus = 0;
    }

    // Increment to next dot/scany. 341 dots per scanline, 262 scanlines per
    // frame. Scanline 261 is represented as -1.
    if (++dot == 341) {
      dot = 0;
      scany++;
      scany %= 262;
    }
  }
  goto loop;

shutdown:
#ifndef SMOLNES_HEADLESS_TRACE
  audio_stop();
#else
  if (headless_pcm_log) fclose(headless_pcm_log);
  if (headless_mix_pcm) fclose(headless_mix_pcm);
  if (headless_dmc_pcm) fclose(headless_dmc_pcm);
  if (headless_apu_time_log) fclose(headless_apu_time_log);
#endif
#ifdef SMOLNES_FRAME_TIME_DIAGNOSTICS
  if (frame_timing_log)
    fclose(frame_timing_log);
#endif
#ifndef SMOLNES_HEADLESS_TRACE
  CancelWaitableTimer(frame_timer_16);
  CancelWaitableTimer(frame_timer_50);
  CloseHandle(frame_timer_16);
  CloseHandle(frame_timer_50);
  frame_timer_16 = frame_timer_50 = NULL;
#endif
  return 0;
}

#ifdef SMOLNES_HEADLESS_TRACE
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command,
                   int show_state) {
  char *path;
  (void)instance; (void)previous; (void)show_state;
  while (*command == ' ') command++;
  if (*command == '"') {
    path = ++command;
    while (*command && *command != '"') command++;
  } else {
    path = command;
    while (*command && *command != ' ') command++;
  }
  *command = 0;
  if (!*path) return 2;
  return run_emulator(path);
}
#else
void mainCRTStartup(void) {
  char *path = select_rom_file();
  if (!path)
    ExitProcess(0);
  ExitProcess((UINT)run_emulator(path));
}
#endif
