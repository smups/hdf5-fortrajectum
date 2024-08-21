const std = @import("std");
const Allocator = std.mem.Allocator;

 pub fn build(b: *std.Build) !void {
    // Initialise allocator
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const alloc = gpa.allocator();

    // Get user-supplied target and optimize functions
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const root_dir = b.build_root.handle;
    const root_path = try root_dir.realpathAlloc(alloc, ".");

    // Add autogen step
    const autogen_step = b.step("autogen", "generate configuration script");
    const autogen_run = b.addSystemCommand(&.{ try root_dir.realpathAlloc(alloc, "autogen.sh") });
    const autogen_out = autogen_run.captureStdOut();
    autogen_step.dependOn(&b.addInstallFile(autogen_out, "hdf5-autogen.log").step);
    autogen_step.dependOn(&autogen_run.step);
    
    // Add a configure step
    const config_step = b.step("configure", "configure hdf5 for building on this machine");
    const config_run = b.addSystemCommand(&.{ try root_dir.realpathAlloc(alloc, "configure") });
    const config_out = config_run.captureStdOut();
    config_run.addArgs(&.{
        "--enable-cxx",
        b.fmt("--srcdir={s}", .{ root_path }),
    });

    // Capture the output to create a dependency for the main code
    config_run.step.dependOn(&autogen_run.step);
    config_step.dependOn(&b.addInstallFile(config_out, "hdf5-config.log").step);

    const wf = b.addWriteFiles();
    wf.step.dependOn(&config_run.step);
    _ = wf.addCopyFile(b.path("src/H5pubconf.h"), "H5pubconf.h");
    _ = wf.addCopyFile(b.path("src/H5config.h"), "H5config.h");
    const build_settings = wf.addCopyFile(b.path("src/H5build_settings.c"), "H5build_settings.c");
   
    // Compile hdf5
    const hdf5 = b.addStaticLibrary(.{
        .name = "hdf5-fortrajectum",
        .target = target,
        .optimize = optimize
    });

    // Depend on configure step
    hdf5.step.dependOn(config_step);

    // Add headers
    hdf5.addIncludePath(b.path("src"));
    hdf5.addIncludePath(b.path("src/H5FDsubfiling"));

    // Add source
    const c_flags = &.{
        "-std=gnu99",
        "-m64",
        "-Wno-error=incompatible-pointer-types-discards-qualifiers",
        "-Wno-error=implicit-function-declaration",
        "-Wno-error=int-conversion",
    };
    hdf5.addCSourceFiles(.{ .files = hdf5_src, .flags = c_flags });
    hdf5.addCSourceFile(.{ .file = build_settings, .flags = c_flags });

    // Link dependencies
    hdf5.linkLibC();
    hdf5.linkSystemLibrary("sz");
    hdf5.linkSystemLibrary("z");
    hdf5.linkSystemLibrary("dl");
    hdf5.linkSystemLibrary("m");
    hdf5.linkSystemLibrary("rt");

    // Install Headers
    hdf5.installHeadersDirectory(b.path("src"), "", .{});
    hdf5.installHeadersDirectory(b.path("src/H5FDsubfiling/"), "", .{});
    hdf5.installHeadersDirectory(wf.getDirectory(), "", .{});
    
    // Compile hdf5cpp
    const hdf5cpp = b.addStaticLibrary(.{
        .name = "hdf5cpp-fortrajectum",
        .optimize = optimize,
        .target = target
    });

    // Depend on configure step
    hdf5cpp.step.dependOn(config_step);

    // Add headers
    hdf5cpp.addIncludePath(b.path("src"));
    hdf5cpp.addIncludePath(b.path("src/H5FDsubfiling"));
    hdf5cpp.addIncludePath(b.path("c++/src"));

    // Link stdc++
    hdf5cpp.linkLibCpp();

    // Add c++ source
    const cpp_flags = &.{ "-std=c++11" };
    hdf5cpp.addCSourceFiles(.{ .files = hdfcpp_src, .flags = cpp_flags });

    // Install headers
    hdf5cpp.installHeadersDirectory(b.path("c++/src"), "", .{});

    // Install binary
    b.installArtifact(hdf5cpp);
    b.installArtifact(hdf5);
}

const hdf5_src = &.{
    "src/H5Abtree2.c",
    "src/H5A.c",
    "src/H5AC.c",
    "src/H5ACdbg.c",
    "src/H5ACmpio.c",
    "src/H5ACproxy_entry.c",
    "src/H5Adense.c",
    "src/H5Adeprec.c",
    "src/H5Aint.c",
    "src/H5Atest.c",
    "src/H5B2.c",
    "src/H5B2cache.c",
    "src/H5B2dbg.c",
    "src/H5B2hdr.c",
    "src/H5B2int.c",
    "src/H5B2internal.c",
    "src/H5B2leaf.c",
    "src/H5B2stat.c",
    "src/H5B2test.c",
    "src/H5B.c",
    "src/H5Bcache.c",
    "src/H5Bdbg.c",
    "src/H5build_settings.c",
    "src/H5.c",
    "src/H5C.c",
    "src/H5Cdbg.c",
    "src/H5Centry.c",
    "src/H5Cepoch.c",
    "src/H5checksum.c",
    "src/H5Cimage.c",
    "src/H5Cint.c",
    "src/H5Clog.c",
    "src/H5Clog_json.c",
    "src/H5Clog_trace.c",
    "src/H5Cmpio.c",
    "src/H5Cprefetched.c",
    "src/H5Cquery.c",
    "src/H5CS.c",
    "src/H5Ctag.c",
    "src/H5Ctest.c",
    "src/H5CX.c",
    "src/H5dbg.c",
    "src/H5Dbtree2.c",
    "src/H5Dbtree.c",
    "src/H5D.c",
    "src/H5Dchunk.c",
    "src/H5Dcompact.c",
    "src/H5Dcontig.c",
    "src/H5Ddbg.c",
    "src/H5Ddeprec.c",
    "src/H5Dearray.c",
    "src/H5Defl.c",
    "src/H5Dfarray.c",
    "src/H5Dfill.c",
    "src/H5Dint.c",
    "src/H5Dio.c",
    "src/H5Dlayout.c",
    "src/H5Dmpio.c",
    "src/H5Dnone.c",
    "src/H5Doh.c",
    "src/H5Dscatgath.c",
    "src/H5Dselect.c",
    "src/H5Dsingle.c",
    "src/H5Dtest.c",
    "src/H5Dvirtual.c",
    "src/H5EA.c",
    "src/H5EAcache.c",
    "src/H5EAdbg.c",
    "src/H5EAdblkpage.c",
    "src/H5EAdblock.c",
    "src/H5EAhdr.c",
    "src/H5EAiblock.c",
    "src/H5EAint.c",
    "src/H5EAsblock.c",
    "src/H5EAstat.c",
    "src/H5EAtest.c",
    "src/H5E.c",
    "src/H5Edeprec.c",
    "src/H5Eint.c",
    "src/H5ES.c",
    "src/H5ESevent.c",
    "src/H5ESint.c",
    "src/H5ESlist.c",
    "src/H5FA.c",
    "src/H5FAcache.c",
    "src/H5Faccum.c",
    "src/H5FAdbg.c",
    "src/H5FAdblkpage.c",
    "src/H5FAdblock.c",
    "src/H5FAhdr.c",
    "src/H5FAint.c",
    "src/H5FAstat.c",
    "src/H5FAtest.c",
    "src/H5F.c",
    "src/H5Fcwfs.c",
    "src/H5Fdbg.c",
    "src/H5FD.c",
    "src/H5FDcore.c",
    "src/H5FDdirect.c",
    "src/H5Fdeprec.c",
    "src/H5FDfamily.c",
    "src/H5FDhdfs.c",
    "src/H5FDint.c",
    "src/H5FDlog.c",
    "src/H5FDmirror.c",
    "src/H5FDmpi.c",
    "src/H5FDmpio.c",
    "src/H5FDmulti.c",
    "src/H5FDonion.c",
    "src/H5FDonion_header.c",
    "src/H5FDonion_history.c",
    "src/H5FDonion_index.c",
    "src/H5FDperform.c",
    "src/H5FDros3.c",
    "src/H5FDs3comms.c",
    "src/H5FDsec2.c",
    "src/H5FDspace.c",
    "src/H5FDsplitter.c",
    "src/H5FDstdio.c",
    "src/H5FDtest.c",
    "src/H5FDwindows.c",
    "src/H5Fefc.c",
    "src/H5Ffake.c",
    "src/H5Fint.c",
    "src/H5Fio.c",
    "src/H5FL.c",
    "src/H5Fmount.c",
    "src/H5Fmpi.c",
    "src/H5FO.c",
    "src/H5Fquery.c",
    "src/H5FS.c",
    "src/H5FScache.c",
    "src/H5FSdbg.c",
    "src/H5Fsfile.c",
    "src/H5FSint.c",
    "src/H5Fspace.c",
    "src/H5FSsection.c",
    "src/H5FSstat.c",
    "src/H5FStest.c",
    "src/H5Fsuper.c",
    "src/H5Fsuper_cache.c",
    "src/H5Ftest.c",
    "src/H5Gbtree2.c",
    "src/H5G.c",
    "src/H5Gcache.c",
    "src/H5Gcompact.c",
    "src/H5Gdense.c",
    "src/H5Gdeprec.c",
    "src/H5Gent.c",
    "src/H5Gint.c",
    "src/H5Glink.c",
    "src/H5Gloc.c",
    "src/H5Gname.c",
    "src/H5Gnode.c",
    "src/H5Gobj.c",
    "src/H5Goh.c",
    "src/H5Groot.c",
    "src/H5Gstab.c",
    "src/H5Gtest.c",
    "src/H5Gtraverse.c",
    "src/H5HFbtree2.c",
    "src/H5HF.c",
    "src/H5HFcache.c",
    "src/H5HFdbg.c",
    "src/H5HFdblock.c",
    "src/H5HFdtable.c",
    "src/H5HFhdr.c",
    "src/H5HFhuge.c",
    "src/H5HFiblock.c",
    "src/H5HFiter.c",
    "src/H5HFman.c",
    "src/H5HFsection.c",
    "src/H5HFspace.c",
    "src/H5HFstat.c",
    "src/H5HFtest.c",
    "src/H5HFtiny.c",
    "src/H5HG.c",
    "src/H5HGcache.c",
    "src/H5HGdbg.c",
    "src/H5HGquery.c",
    "src/H5HL.c",
    "src/H5HLcache.c",
    "src/H5HLdbg.c",
    "src/H5HLdblk.c",
    "src/H5HLint.c",
    "src/H5HLprfx.c",
    "src/H5I.c",
    "src/H5Idbg.c",
    "src/H5Iint.c",
    "src/H5Itest.c",
    "src/H5L.c",
    "src/H5Ldeprec.c",
    "src/H5Lexternal.c",
    "src/H5Lint.c",
    "src/H5M.c",
    "src/H5MFaggr.c",
    "src/H5MF.c",
    "src/H5MFdbg.c",
    "src/H5MFsection.c",
    "src/H5MM.c",
    "src/H5mpi.c",
    "src/H5Oainfo.c",
    "src/H5Oalloc.c",
    "src/H5Oattr.c",
    "src/H5Oattribute.c",
    "src/H5Obogus.c",
    "src/H5Obtreek.c",
    "src/H5O.c",
    "src/H5Ocache.c",
    "src/H5Ocache_image.c",
    "src/H5Ochunk.c",
    "src/H5Ocont.c",
    "src/H5Ocopy.c",
    "src/H5Ocopy_ref.c",
    "src/H5Odbg.c",
    "src/H5Odeprec.c",
    "src/H5Odrvinfo.c",
    "src/H5Odtype.c",
    "src/H5Oefl.c",
    "src/H5Ofill.c",
    "src/H5Oflush.c",
    "src/H5Ofsinfo.c",
    "src/H5Oginfo.c",
    "src/H5Oint.c",
    "src/H5Olayout.c",
    "src/H5Olinfo.c",
    "src/H5Olink.c",
    "src/H5Omessage.c",
    "src/H5Omtime.c",
    "src/H5Oname.c",
    "src/H5Onull.c",
    "src/H5Opline.c",
    "src/H5Orefcount.c",
    "src/H5Osdspace.c",
    "src/H5Oshared.c",
    "src/H5Oshmesg.c",
    "src/H5Ostab.c",
    "src/H5Otest.c",
    "src/H5Ounknown.c",
    "src/H5Pacpl.c",
    "src/H5PB.c",
    "src/H5P.c",
    "src/H5Pdapl.c",
    "src/H5Pdcpl.c",
    "src/H5Pdeprec.c",
    "src/H5Pdxpl.c",
    "src/H5Pencdec.c",
    "src/H5Pfapl.c",
    "src/H5Pfcpl.c",
    "src/H5Pfmpl.c",
    "src/H5Pgcpl.c",
    "src/H5Pint.c",
    "src/H5Plapl.c",
    "src/H5PL.c",
    "src/H5Plcpl.c",
    "src/H5PLint.c",
    "src/H5PLpath.c",
    "src/H5PLplugin_cache.c",
    "src/H5Pmapl.c",
    "src/H5Pmcpl.c",
    "src/H5Pocpl.c",
    "src/H5Pocpypl.c",
    "src/H5Pstrcpl.c",
    "src/H5Ptest.c",
    "src/H5R.c",
    "src/H5Rdeprec.c",
    "src/H5Rint.c",
    "src/H5RS.c",
    "src/H5Sall.c",
    "src/H5S.c",
    "src/H5Sdbg.c",
    "src/H5Sdeprec.c",
    "src/H5Shyper.c",
    "src/H5SL.c",
    "src/H5SMbtree2.c",
    "src/H5SM.c",
    "src/H5SMcache.c",
    "src/H5SMmessage.c",
    "src/H5Smpio.c",
    "src/H5SMtest.c",
    "src/H5Snone.c",
    "src/H5Spoint.c",
    "src/H5Sselect.c",
    "src/H5Stest.c",
    "src/H5system.c",
    "src/H5Tarray.c",
    "src/H5Tbit.c",
    "src/H5T.c",
    "src/H5Tcommit.c",
    "src/H5Tcompound.c",
    "src/H5Tconv.c",
    "src/H5Tcset.c",
    "src/H5Tdbg.c",
    "src/H5Tdeprec.c",
    "src/H5Tenum.c",
    "src/H5Tfields.c",
    "src/H5Tfixed.c",
    "src/H5Tfloat.c",
    "src/H5timer.c",
    "src/H5Tinit_float.c",
    "src/H5Tnative.c",
    "src/H5Toffset.c",
    "src/H5Toh.c",
    "src/H5Topaque.c",
    "src/H5Torder.c",
    "src/H5Tpad.c",
    "src/H5Tprecis.c",
    "src/H5trace.c",
    "src/H5Tref.c",
    "src/H5TS.c",
    "src/H5Tstrpad.c",
    "src/H5Tvisit.c",
    "src/H5Tvlen.c",
    "src/H5UC.c",
    "src/H5VL.c",
    "src/H5VLcallback.c",
    "src/H5VLdyn_ops.c",
    "src/H5VLint.c",
    "src/H5VLnative_attr.c",
    "src/H5VLnative_blob.c",
    "src/H5VLnative.c",
    "src/H5VLnative_dataset.c",
    "src/H5VLnative_datatype.c",
    "src/H5VLnative_file.c",
    "src/H5VLnative_group.c",
    "src/H5VLnative_introspect.c",
    "src/H5VLnative_link.c",
    "src/H5VLnative_object.c",
    "src/H5VLnative_token.c",
    "src/H5VLpassthru.c",
    "src/H5VLtest.c",
    "src/H5VM.c",
    "src/H5WB.c",
    "src/H5Z.c",
    "src/H5Zdeflate.c",
    "src/H5Zfletcher32.c",
    "src/H5Znbit.c",
    "src/H5Zscaleoffset.c",
    "src/H5Zshuffle.c",
    "src/H5Zszip.c",
    "src/H5Ztrans.c"
};

const hdfcpp_src = &.{
    "c++/src/H5AbstractDs.cpp",
    "c++/src/H5ArrayType.cpp",
    "c++/src/H5AtomType.cpp",
    "c++/src/H5Attribute.cpp",
    "c++/src/H5CommonFG.cpp",
    "c++/src/H5CompType.cpp",
    "c++/src/H5DaccProp.cpp",
    "c++/src/H5DataSet.cpp",
    "c++/src/H5DataSpace.cpp",
    "c++/src/H5DataType.cpp",
    "c++/src/H5DcreatProp.cpp",
    "c++/src/H5DxferProp.cpp",
    "c++/src/H5EnumType.cpp",
    "c++/src/H5Exception.cpp",
    "c++/src/H5FaccProp.cpp",
    "c++/src/H5FcreatProp.cpp",
    "c++/src/H5File.cpp",
    "c++/src/H5FloatType.cpp",
    "c++/src/H5Group.cpp",
    "c++/src/H5IdComponent.cpp",
    "c++/src/H5IntType.cpp",
    "c++/src/H5LaccProp.cpp",
    "c++/src/H5LcreatProp.cpp",
    "c++/src/H5Library.cpp",
    "c++/src/H5Location.cpp",
    "c++/src/H5Object.cpp",
    "c++/src/H5OcreatProp.cpp",
    "c++/src/H5PredType.cpp",
    "c++/src/H5PropList.cpp",
    "c++/src/H5StrType.cpp",
    "c++/src/H5VarLenType.cpp",
};
