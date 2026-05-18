#pragma once
#pragma GCC optimize("O3")

namespace pixeler
{
  namespace mime
  {
    enum type
    {
      html,
      htm,
      css,
      txt,
      js,
      mjs,
      json,
      png,
      gif,
      jpg,
      ico,
      svg,
      ttf,
      otf,
      woff,
      woff2,
      eot,
      sfnt,
      xml,
      pdf,
      zip,
      gz,
      appcache,
      none,
      maxType
    };

    struct Entry
    {
      const char endsWith[16];
      const char mimeType[32];
    };

    extern const Entry mimeTable[maxType];
  }  // namespace mime
}  // namespace pixeler
