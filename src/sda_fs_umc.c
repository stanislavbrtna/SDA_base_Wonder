/*
Copyright (c) 2018 Stanislav Brtna

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "sda_platform.h"

uint8_t sd_mounted;

/*konvence jest: pokud ok, tak vracíme nulu*/

uint8_t svp_fopen_read(svp_file *fp, uint8_t *fname) {

  if (!sd_mounted) {
    return 0;
  }

  fp->fPointerRes = f_open(&(fp->fPointer), (char*)fname, FA_READ);
  if (fp->fPointerRes == FR_OK) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t svp_fopen_rw(svp_file *fp, uint8_t *fname) {

  if (!sd_mounted) {
    return 0;
  }
  fp->fPointerRes = f_open(&(fp->fPointer), (char*)fname, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
  if (fp->fPointerRes == FR_OK) {
    return 1;
  } else {
    return 0;
  }
}


uint8_t svp_fexists(uint8_t *fname) {
  svp_file fp;

  if (!sd_mounted) {
    return 0;
  }

  fp.fPointerRes = f_stat((char *)fname, 0);

  if (fp.fPointerRes == FR_OK) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t svp_fread_u8(svp_file *fp) {
  uint8_t x;

  f_read(&(fp->fPointer), &x, sizeof(x), (UINT*)&(fp->fPointerRes));
  return x;
}

uint8_t svp_fread(svp_file *fp, void *target, uint32_t size) {
  f_read(&(fp->fPointer), target, size, (UINT*)&(fp->fPointerRes));
  return 0;
}

void svp_fwrite_u8(svp_file *fp, uint8_t val) {
  f_write(&(fp->fPointer), &val, sizeof(uint8_t), (UINT*)&(fp->fPointerRes));
}

void svp_fwrite(svp_file *fp, void *target, uint32_t size) {
  f_write(&(fp->fPointer), target, size, (UINT*)&(fp->fPointerRes));
}

uint8_t svp_feof(svp_file *fp) {
  return f_eof(&(fp->fPointer));
}

uint8_t svp_fclose(svp_file *fp) {
  return f_close(&(fp->fPointer));
}

uint8_t svp_fseek(svp_file *fp, uint32_t offset) {
  f_lseek(&(fp->fPointer), offset);
  return 0;
}

uint32_t svp_get_size(svp_file *fp) {
  return f_size(&(fp->fPointer));
}

int32_t svp_get_mtime(uint8_t *fname) {
  FRESULT fr;
  FILINFO fno;
  
  if (f_stat(fname, &fno) != FR_OK) {
    return 0;
  }

  return sdaTimeGetTimestamp(
    (fno.fdate >> 9) + 1980,
    fno.fdate >> 5 & 15,
    fno.fdate & 31,
    fno.ftime >> 11,
    fno.ftime >> 5 & 63,
    fno.ftime & 63
  );
}

uint32_t svp_get_size_n(uint8_t *fname) {
  FRESULT fr;
  FILINFO fno;
  
  if (f_stat(fname, &fno) != FR_OK) {
    return 0;
  }

  return fno.fsize;
}

uint32_t svp_ftell(svp_file *fp) {
  return f_tell (&(fp->fPointer));
}


void svp_truncate(svp_file *fp) {
  f_truncate (&(fp->fPointer));
}

uint8_t svp_rename(uint8_t *source, uint8_t *dest) {
  FRESULT res;
  res = f_rename((char *)source,(char *) dest);
  if (res) {
    return 0;
  } else {
    return 1;
  }
}


uint8_t exten[8];
static FRESULT res;
static DIR dir;
static uint8_t dir_openned;

uint8_t svp_strcmp_ext(uint8_t *s1, uint8_t *s_ext) {
  uint16_t x = 0;
  uint16_t y = 0;
  //printf("comparing: %s a %s : ", s1, s_ext);
  while(s1[x] != '.') {
    if (s1[x] == 0) {
      if (s_ext[0] == 0) {
        return 1; // if there is no extension, we list all the files
      } else {
        return 0;
      }
      //printf("fail! dot not found\n");
    }
    x++;
  }
  x++;

  while (s_ext[y] != 0) {
    if (s1[x+y] != s_ext[y]) {
      //printf("fail!\n");
      return 0;
    }
    y++;
  }
  //printf("pass!\n");
  return 1;
}


uint8_t svp_extFindNext(uint8_t *outStr, uint16_t len) {
  static FILINFO fno;
  if (!dir_openned) {
    return 0;
  }
  while (1) {
    res = f_readdir(&dir, &fno);
      //printf("dir: %d\n", fno.fname);
      if((res==FR_OK)&&(fno.fname[0] != 0)) {
        if(svp_strcmp_ext((uint8_t *)fno.fname, exten)) {
          sda_strcp((uint8_t *)fno.fname, outStr, len);
          return 1;
        } else {
          continue;
        }
      } else {
      //printf("closing\n");
      f_closedir(&dir);
      dir_openned = 0;
      //printf("closed\n");
      return 0;
      }
    }
}

uint8_t svp_extFind(uint8_t *outStr, uint16_t len, uint8_t *extension, uint8_t *directory){
  sda_strcp(extension, exten, 7);
  if (dir_openned) {
    f_closedir(&dir);
    dir_openned = 0;
  }

  res = f_opendir(&dir, (char *)directory);
  if (res == FR_OK) {
    dir_openned = 1;
    return svp_extFindNext(outStr, len);
  }
  return 0;
}

uint8_t svp_open_dir(svp_dir *dp, uint8_t *path) {
  dp->res = f_opendir(&(dp->dir), (char *)path);
  if (dp->res == FR_OK) {
    return 0;
  } else {
    return 1;
  }

}

uint8_t svp_close_dir(svp_dir *dp) {
  f_closedir(&(dp->dir));
  return 0;
}

uint8_t svp_strcmp(uint8_t *a, uint8_t *b) {
  uint16_t x = 0;
  uint8_t retval = 1;
  //printf("comp: %s a %s\n", a, b);
  while (x < 100) {
    if ((a[x] == 0) || (b[x] == 0)) {
      if (a[x] != b[x]) {
        retval = 0;
      }
      break;
    } else {
      if (a[x] != b[x]){
        retval = 0;
      }
      x++;
    }
  }
  return retval;
}

uint16_t svp_strlen(uint8_t *str) {
  uint16_t x = 0;

  while(str[x] != 0) {
    x++;
  }

  return x + 1; //vrátí len i s terminátorem
}

#ifdef FF_FS_EXFAT
// custom CWD setup

uint8_t currentDir[512];

uint8_t svp_chdir(uint8_t* path) {
  // remove slash on the end of path
  if (path[svp_strlen(path - 1)] == '/' && svp_strlen(path - 1) != 0) {
    path[svp_strlen(path - 1)] = 0;
  }

  FRESULT f = f_chdir((char *)path);

  if (f != FR_OK) {
    printf("%s: chDir Failed!\n", __FUNCTION__);
    return 0;
  }

  // root check
  if (path[0] == '/') {
    // fill
    sda_strcp(path, currentDir, sizeof(currentDir));
  } else {
    if(svp_strcmp(path, (uint8_t *)"..")) {
      // find last slash
      for(uint32_t i = sda_strlen(currentDir); i > 1; i--) {
        if(currentDir[i] == '/') {
          currentDir[i] = 0;
          break;
        }
      }
    } else if(svp_strcmp(path, (uint8_t *)".")) {
      // Do nothing
    } else {
      if(currentDir[sda_strlen(currentDir) - 1] != '/' && sda_strlen(currentDir) != 2) {
        sda_str_add(currentDir, (uint8_t *)"/");
      }
      sda_str_add(currentDir, path);
    }
  }
  //printf("chdir: %s, newCWD: %s\n", path, currentDir);

  return 1;
}

uint8_t svp_getcwd(uint8_t* buf, uint16_t len) {
  sda_strcp(currentDir, buf, len);
  return 0;
}

#else

uint8_t svp_chdir(uint8_t* path) {
  // remove slash on the end of path
  if (path[svp_strlen(path - 1)] == '/' && svp_strlen(path - 1) != 0) {
    path[svp_strlen(path - 1)] = 0;
  }

  f_chdir((char *)path);
  return 0;
}

uint8_t svp_getcwd(uint8_t* buf, uint16_t len) {
  f_getcwd((char *)buf, len);
  return 0;
}

#endif

uint8_t svp_unlink(uint8_t* path) {
  f_unlink ((char *)path);
  return 0;
}

uint8_t svp_rmdir(uint8_t* path) {
  f_unlink ((char *)path);
  return 0;
}

uint8_t svp_is_dir(uint8_t* path) {
  FRESULT fr;
  FILINFO fno;

  fr = f_stat((char *)path, &fno);

  if (fr != FR_OK) {
    printf("Error: svp_is_dir: no such file?\n");
    return 0;
  }

  if (fno.fattrib & AM_DIR) {
    return 1;
  }

  return 0;
}

uint8_t svp_mkdir(uint8_t* path) {
  FRESULT res;
  res = f_mkdir((char *)path);
  if (res) {
    return 0;
  } else {
    return 1;
  }
}

void svp_fsync(svp_file *fp) {
  f_sync(&(fp->fPointer));
}

uint8_t svp_mount() {
  FRESULT fr;

  if (sda_card_inserted() == 0) {
    return 2;
  }

  fr = f_mount(&FatFs, "", 1);

  if(fr != FR_OK) {
    return 1; // mount error
  }else{
    sd_mounted = 1;
    svp_chdir((uint8_t *)"/"); // reset the custom cwd field
    return 0; // mount ok
  }
}

void svp_umount() {
  sd_mounted = 0;
  f_mount(0, "", 0);
}

uint8_t svp_getMounted() {
  return sd_mounted;
}

// because it was mounted before the svp init
void svp_setMounted(uint8_t val) {
  sd_mounted = val;
}

/* File Find example:
uint8_t buffer[32];
uint8_t retval;

printf("extFind: print all C files\n");

retval=svp_extFind(buffer, 30, "c", ".");

while (retval){
  printf("file: %s\n", buffer);
  retval = svp_extFindNext(buffer, 30);
}

*/

