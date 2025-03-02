library(knitr)
dir.create("compiled", showWarnings=FALSE)

for (v in c("1.0", "1.1", "1.2", "1.3", "1.4")) {
    .version <- package_version(v)
    knitr::knit("hdf5.Rmd", output=file.path("compiled", paste0("hdf5-", v, ".md")))
}

for (v in c("1.0", "1.1", "1.2")) {
    .version <- package_version(v)
    knitr::knit("json.Rmd", output=file.path("compiled", paste0("json-", v, ".md")))
}

file.copy("misc.md", file.path("compiled", "misc.md"))
