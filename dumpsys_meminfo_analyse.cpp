#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string.h>
#include <iostream>
class MemoryUsageReaderImpl{
  public:
    void GetProcessMemoryLevels(int pid);
    int ParseInt(char** delimited_string,
                                    const char* delimiter);

    void ParseMemoryLevels(const std::string& memory_info_string);
};

// const char* kDumpsysCommandFormat = "dumpsys meminfo --local --checkin %d";
// const int kCommandMaxLength = 128;
// const int kBufferSize = 1024;
enum MemoryType {
  UNKNOWN,
  PRIVATE_CLEAN,
  PRIVATE_DIRTY,
  ART,
  STACK,
  GRAPHICS,
  CODE,
  OTHERS
};

int main(int argc, char* argv[]){
    MemoryUsageReaderImpl memoryUsageReaderImpl;
    memoryUsageReaderImpl.GetProcessMemoryLevels(123);
}
void MemoryUsageReaderImpl::GetProcessMemoryLevels(
    int pid) {
//   char buffer[kBufferSize];
//   char cmd[kCommandMaxLength];
//   int num_written =
//       snprintf(cmd, kCommandMaxLength, kDumpsysCommandFormat, pid);
//   if (num_written >= kCommandMaxLength) {
//     return;  // TODO error handling.
//   }
//   // TODO: Use BashCommand object which provide central access point to
//   // popen and also shows traces in systrace.
//   std::string output = "";
//   std::unique_ptr<FILE, int (*)(FILE*)> mem_info_file(popen(cmd, "r"), pclose);
//   if (!mem_info_file) {
//     return;  // TODO error handling.
//   }
//   // Skip lines until actual data. Note that before N, "--checkin" is not an
//   // official flag
//   // so the arg parsing logic complains about invalid arguments.
//   do {
//     if (feof(mem_info_file.get())) {
//       return;  // TODO error handling.
//     }
//     fgets(buffer, kBufferSize, mem_info_file.get());
//     // Skip ahead until the header, which is in the format of: "time, (uptime),
//     // (realtime)".
//   } while (strncmp(buffer, "time,", 5) != 0);
//   // Gather the remaining content which should be a comma-delimited string.
//   while (!feof(mem_info_file.get()) &&
//          fgets(buffer, kBufferSize, mem_info_file.get()) != nullptr) {
//     output += buffer;
//   }
  return ParseMemoryLevels("4,2110,com.android.systemui,35732,9668,N/A,45400,33958,4834,N/A,38792,1773,4834,N/A,6607,29173,8493,26162,63828,0,0,11232,11232,1664,1968,12912,16544,0,0,119940,119940,29112,8428,5376,42916,0,0,11320,11320,0,0,0,0,N/A,N/A,N/A,N/A,Dalvik Other,1518,0,100,0,1516,0,0,N/A,Stack,36,0,4,0,36,0,0,N/A,Cursor,0,0,0,0,0,0,0,N/A,Ashmem,126,0,32,16,116,0,0,N/A,Gfx dev,0,0,0,0,0,0,0,N/A,Other dev,28,0,144,0,0,28,0,N/A,.so mmap,2890,128,2636,29820,152,128,0,N/A,.jar mmap,2686,172,0,26728,0,172,0,N/A,.apk mmap,12828,10932,0,44284,0,10932,0,N/A,.ttf mmap,155,0,0,488,0,0,0,N/A,.dex mmap,182,0,8,392,8,0,0,N/A,.oat mmap,1157,0,44,16092,0,0,0,N/A,.art mmap,2494,0,8548,180,2168,0,0,N/A,Other mmap,952,0,1012,1940,272,60,0,N/A,EGL mtrack,0,0,0,0,0,0,0,N/A,GL mtrack,0,0,0,0,0,0,0,N/A,Other mtrack,0,0,0,0,0,0,0,N/A,831,4,16,0,12,0,176,71,3,0,0,0,0,0");
}
void MemoryUsageReaderImpl::ParseMemoryLevels(
    const std::string& memory_info_string) {
  std::unique_ptr<char, void (*)(void*)> delimited_memory_info(
      strdup(memory_info_string.c_str()), std::free);
  char* temp_memory_info_ptr = delimited_memory_info.get();
  char* result;
  int32_t java_private = 0, native_private = 0, stack = 0, graphics = 0,
          code = 0, other_private = 0;
  // Version check.
  int version = ParseInt(&temp_memory_info_ptr, ",");
  int regularStatsFieldCount = 4;
  int privateDirtyStartIndex =
      30;  // index before the private dirty category begins.
  int privateCleanStartIndex =
      34;  // index before the private clean category begins.
  int otherStatsFieldCount;
  int otherStatsStartIndex;
  if (version == 4) {
    // New categories (e.g. swappable memory) have been inserted before the
    // other stats categories
    // compared to version 3, so we only have to move forward the
    // otherStatsStartIndex.
    otherStatsStartIndex = 47;
    otherStatsFieldCount = 8;
  } else if (version == 3) {
    otherStatsStartIndex = 39;
    otherStatsFieldCount = 6;
  } else {
    // Older versions predating Kitkat are unsupported - early return.
    return;
  }
  // The logic below extracts the private clean+dirty memory from the
  // comma-delimited string,
  // which starts with: (the capitalized fields above are the ones we need)
  //   {version (parsed above), pid, process_name,}
  // then in groups of 4, the main heap info: (e.g. pss, shared dirty/clean,
  // private dirty/clean)
  //    {NATIVE, DALVIK, other, total,}
  // follow by the other stats, in groups of the number defined in
  // otherStatsFieldCount:
  //    {stats_label, total_pss, swappable_pss, shared_dirty, shared_clean,
  //    PRIVATE_DIRTY,
  //     PRIVATE_CLEAN,...}
  //
  // Note that the total private memory from this format is slightly less than
  // the human-readable
  // dumpsys meminfo version, as that accounts for a small amount of "unknown"
  // memory where the
  // "--checkin" version does not.
  int currentIndex = 0;
  while (true) {
    result = strsep(&temp_memory_info_ptr, ",");
    currentIndex++;
    if (result == nullptr) {
      // Reached the end of the output.
      break;
    }
    int memory_type = UNKNOWN;
    if (currentIndex >= otherStatsStartIndex) {
      if (strcmp(result, "Dalvik Other") == 0 ||
          strcmp(result, "Ashmem") == 0 || strcmp(result, "Cursor") == 0 ||
          strcmp(result, "Other dev") == 0 ||
          strcmp(result, "Other mmap") == 0 ||
          strcmp(result, "Other mtrack") == 0 ||
          strcmp(result, "Unknown") == 0) {
        memory_type = OTHERS;
      } else if (strcmp(result, "Stack") == 0) {
        memory_type = STACK;
      } else if (strcmp(result, ".art mmap") == 0) {
        memory_type = ART;
      } else if (strcmp(result, "Gfx dev") == 0 ||
                 strcmp(result, "EGL mtrack") == 0 ||
                 strcmp(result, "GL mtrack") == 0) {
        memory_type = GRAPHICS;
      } else if (strcmp(result, ".so mmap") == 0 ||
                 strcmp(result, ".jar mmap") == 0 ||
                 strcmp(result, ".apk mmap") == 0 ||
                 strcmp(result, ".ttf mmap") == 0 ||
                 strcmp(result, ".dex mmap") == 0 ||
                 strcmp(result, ".oat mmap") == 0) {
        memory_type = CODE;
      }
    } else if (currentIndex == privateCleanStartIndex) {
      memory_type = PRIVATE_CLEAN;
    } else if (currentIndex == privateDirtyStartIndex) {
      memory_type = PRIVATE_DIRTY;
    }
    if (memory_type == PRIVATE_CLEAN) {
      other_private +=
          ParseInt(&temp_memory_info_ptr, ",");  // native private clean.
      other_private +=
          ParseInt(&temp_memory_info_ptr, ",");  // dalvik private clean.
      strsep(&temp_memory_info_ptr,
             ",");  // UNUSED - other private clean total.
      strsep(&temp_memory_info_ptr, ",");  // UNUSED - total private clean.
      currentIndex += regularStatsFieldCount;
    } else if (memory_type == PRIVATE_DIRTY) {
      native_private +=
          ParseInt(&temp_memory_info_ptr, ",");  // native private dirty.
      java_private +=
          ParseInt(&temp_memory_info_ptr, ",");  // dalvik private dirty.
      strsep(&temp_memory_info_ptr,
             ",");  // UNUSED - other private dirty are tracked separately.
      strsep(&temp_memory_info_ptr, ",");  // UNUSED - total private dirty.
      currentIndex += regularStatsFieldCount;
    } else if (memory_type != UNKNOWN) {
      strsep(&temp_memory_info_ptr, ",");  // UNUSED - total pss.
      strsep(&temp_memory_info_ptr, ",");  // UNUSED - pss clean.
      strsep(&temp_memory_info_ptr, ",");  // UNUSED - shared dirty.
      strsep(&temp_memory_info_ptr, ",");  // UNUSED - shared clean.
      // Parse out private dirty and private clean.
      switch (memory_type) {
        case OTHERS:
          other_private += ParseInt(&temp_memory_info_ptr, ",");
          other_private += ParseInt(&temp_memory_info_ptr, ",");
          break;
        case STACK:
          stack += ParseInt(&temp_memory_info_ptr, ",");
          // Note that stack's private clean is treated as private others in
          // dumpsys.
          other_private += ParseInt(&temp_memory_info_ptr, ",");
          break;
        case ART:
          java_private += ParseInt(&temp_memory_info_ptr, ",");
          java_private += ParseInt(&temp_memory_info_ptr, ",");
          break;
        case GRAPHICS:
          graphics += ParseInt(&temp_memory_info_ptr, ",");
          graphics += ParseInt(&temp_memory_info_ptr, ",");
          break;
        case CODE:
          code += ParseInt(&temp_memory_info_ptr, ",");
          code += ParseInt(&temp_memory_info_ptr, ",");
          break;
      }
      currentIndex += otherStatsFieldCount;
    }
  }


//   data->set_java_mem(java_private);
//   data->set_native_mem(native_private);
//   data->set_stack_mem(stack);
//   data->set_graphics_mem(graphics);
//   data->set_code_mem(code);
//   data->set_others_mem(other_private);
//   data->set_total_mem(java_private + native_private + stack + graphics + code +
//                       other_private);
std::cout<<"java_private"<<std::endl;
std::cout<<java_private<<std::endl;
std::cout<<"==============="<<std::endl;
std::cout<<"native_private"<<std::endl;
std::cout<<native_private<<std::endl;
std::cout<<"==============="<<std::endl;
std::cout<<"stack"<<std::endl;
std::cout<<stack<<std::endl;
std::cout<<"==============="<<std::endl;
std::cout<<"graphics"<<std::endl;
std::cout<<graphics<<std::endl;
std::cout<<"==============="<<std::endl;
std::cout<<"code"<<std::endl;
std::cout<<code<<std::endl;
std::cout<<"==============="<<std::endl;
std::cout<<"other_private"<<std::endl;
std::cout<<other_private<<std::endl;
std::cout<<"==============="<<std::endl;
std::cout<<"java_private + native_private + stack + graphics + code + other_private"<<std::endl;
std::cout<<java_private + native_private + stack + graphics + code + other_private<<std::endl;
std::cout<<"==============="<<std::endl;

  return;
}
int MemoryUsageReaderImpl::ParseInt(char** delimited_string,
                                    const char* delimiter) {
  char* result = strsep(delimited_string, delimiter);
  if (result == nullptr) {
    return 0;
  } else {
    return strtol(result, nullptr, 10);
  }
}
