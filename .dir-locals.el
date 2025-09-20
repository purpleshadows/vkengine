((c++-mode
  . ((lsp-clients-clangd-args
      . ("--compile-commands-dir=build"
         ;; Only needed if you use this GCC to build;
         ;; remove if you're using MSVC/Clang instead.
         "--query-driver=C:/TDM-GCC-64/bin/*"))))
 (c-mode
  . ((lsp-clients-clangd-args
      . ("--compile-commands-dir=build"
         "--query-driver=C:/TDM-GCC-64/bin/*")))))
