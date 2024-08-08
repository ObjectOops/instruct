#include <filesystem>
#include <exception>
#include <typeinfo>
#include <fstream>
#include <memory>

#include "loguru.hpp"
#include "httplib.h"

#include "archive_entry.h"
#include "archive.h"

#include "constants.hpp"
#include "logging.hpp"
#include "setup.hpp"
#include "data.hpp"

namespace instruct {

static setup::SetupError setupError {};

setup::SetupError &setup::getSetupError() {
    return setupError;
}

bool setup::setupIncomplete() {
    bool pathExists {std::filesystem::exists(constants::DATA_DIR)};
    bool isDir {std::filesystem::is_directory(constants::DATA_DIR)};
    DLOG_F(INFO, "Data path stat: %d %d", pathExists, isDir);
    return !(pathExists && isDir);
}

bool setup::createDataDir() {
    bool success {
        std::filesystem::create_directory(constants::DATA_DIR, setupError.errCode) 
        && std::filesystem::create_directory(
            constants::OPENVSCODE_SERVER_DIR, setupError.errCode
        )
    };
    if (!success) {
        setupError.msg = "Failed to create data directory.";
    }
    return success;
}

bool setup::populateDataDir() {
    std::filesystem::path paths [] {
        constants::INSTRUCTOR_CONFIG, 
        constants::STUDENTS_CONFIG, 
        constants::TESTS_CONFIG, 
        constants::UI_CONFIG
    };
    for (std::filesystem::path &path : paths) {
        try {
            std::ofstream fout;
            fout.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            fout.open(path);
            fout.close();
        } catch (const std::exception &e) {
            setupError.errCode = std::make_error_code(std::errc::io_error);
            setupError.exType = typeid(e).name();
            setupError.exMsg = e.what();
            setupError.msg = 
                "Failed to create file `" + std::string {path} + "`.";
            return false;
        }
    }
    return true;
}

bool setup::setDefaults() {
    try {
        DLOG_F(INFO, "Assigning default instructor data.");
        IData::instructorData->set_authHost("0.0.0.0");
        IData::instructorData->set_authPort(8000);
        IData::instructorData->set_codePort(3000);
        IData::instructorData->set_firstTime(true);
        IData::instructorData->set_ovscsVersion("none");
        DLOG_F(INFO, "Saved default data.");

        DLOG_F(INFO, "Assigning default students data.");
        SData::studentsData->set_authHost("0.0.0.0");
        SData::studentsData->set_authPort(8001);
        SData::studentsData->set_codePorts({});
        SData::studentsData->set_codePortRange({3001, 4000});
        SData::studentsData->set_useRandomPorts(true);
        #if DEBUG
        uuids::uuid debugStudentUUID {
            uuids::uuid::from_string("9d5b69cc-1aca-46bd-940a-de1f110357a9").value()
        };
        SData::studentsData->set_students({{
            debugStudentUUID, 
            {
                debugStudentUUID, 
                "Placeholder Student", 
                "7bcbc837d7bf10efc0019386dc1eb29637669eac0a6285ad441f31f97f72f25c", 
                "3805596074", 
                false
            }
        }});
        #endif
        DLOG_F(INFO, "Saved default data.");
        
        DLOG_F(INFO, "Assigning default tests data.");
        TData::testsData->set_selectedTestUUIDs({});
        #if DEBUG
        uuids::uuid debugTestUUID {
            uuids::uuid::from_string("78127002-2f3d-4655-83fd-d0f6b7e25b95").value()
        };
        TData::testsData->set_tests({{
            debugTestUUID, 
            {
                debugTestUUID, 
                "Placeholder Test", 
                "echo 'Hello, world!'", 
                "echo 'Hello, world!'", 
                0.1 + 0.2
            }
        }});
        #endif
        DLOG_F(INFO, "Saved default data.");
        
        DLOG_F(INFO, "Assigning default UI data.");
        UData::uiData->set_alwaysShowStudentUUIDs(false);
        UData::uiData->set_alwaysShowTestUUIDs(false);
        UData::uiData->set_studentPaneWidth(36);
        DLOG_F(INFO, "Saved default data.");
    } catch (const std::exception &e) {
        setupError.errCode = std::make_error_code(std::errc::io_error);
        setupError.exType = typeid(e).name();
        setupError.exMsg = e.what();
        setupError.msg = "Failed to set defaults.";
        return false;
    }
    return true;
}

void setup::deleteDataDir() {
    DLOG_F(INFO, "Deleted data dir.");
    std::filesystem::remove_all(constants::DATA_DIR);
}

bool setup::deleteOVSCSDirContents() {
    try {
        std::filesystem::remove_all(constants::OPENVSCODE_SERVER_DIR);
        std::filesystem::create_directory(constants::OPENVSCODE_SERVER_DIR);
        return true;
    } catch (const std::exception &e) {
        log::logExceptionWarning(e);
        return false;
    }
}

bool setup::downloadOVSCS(
    const std::string &ovscsVersion, 
    std::atomic<uint64_t> &progress, 
    std::atomic<uint64_t> &totalProgress, 
    int selectedPlatform
) {
    try {
        std::string route {constants::OPENVSCODE_SERVER_ROUTE_FORMAT};
        auto findAndReplaceAll {[] (
            std::string &str, 
            std::string pat, 
            const std::string &rep
        ) {
            std::size_t patLen {pat.length()};
            std::size_t idx {};
            while (true) {
                idx = str.find(pat, idx);
                if (idx == std::string::npos) {
                    break;
                }
                str.replace(idx, patLen, rep);
                idx += patLen;
            }
        }};
        findAndReplaceAll(route, "${VERSION}", ovscsVersion);
        findAndReplaceAll(
            route, "${PLATFORM}", constants::OPENVSCODE_SERVER_PLATFORM.at(selectedPlatform)
        );
        
        DLOG_F(
            INFO, 
            "Attempting download from %s%s.", 
            constants::OPENVSCODE_SERVER_HOST.c_str(), 
            route.c_str()
        );
        
        bool downloaded {false};
        httplib::SSLClient client {constants::OPENVSCODE_SERVER_HOST};
        client.set_follow_location(true);
        
        std::ofstream fout {constants::OPENVSCODE_SERVER_ARCHIVE};
        httplib::Result res {client.Get(route, 
            [&] (const char *data, size_t dataLen) {
                fout.write(data, dataLen);
                return true;
            }, 
            [&] (uint64_t len, uint64_t total) {
                progress = len;
                totalProgress = total;
                return true;
            }
        )};
        if (res.error() != httplib::Error::Success) {
            LOG_F(WARNING, "HTTPS GET error code: %d", static_cast<int>(res.error()));
        } else if (res->status != httplib::StatusCode::OK_200) {
            LOG_F(WARNING, "HTTPS GET status code: %d", res->status);
        } else {
            downloaded = true;
        }
        fout.close();

        return downloaded;
    } catch (const std::exception &e) {
        log::logExceptionWarning(e);
        return false;
    }
}

static bool extract(const char *);
static int copy_data(archive *, archive *);

bool setup::unpackOVSCSTarball() {
    if (!extract(constants::OPENVSCODE_SERVER_ARCHIVE.c_str())) {
        LOG_F(ERROR, "Archive extraction failed.");
        return false;
    }
    return true;
}

// Reference: 
// https://github.com/libarchive/libarchive/
// blob/586a9645102c87d83919015a9e2e49aec7d47a63/examples/untar.c

static bool extract(const char *filename) {
	archive *a;
	archive *ext;
	archive_entry *entry;
	int r;
    
    auto cleanArchive {[&] {
        archive_read_close(a);
        archive_read_free(a);
        
        archive_write_close(ext);
        archive_write_free(ext);
    }};

	a = archive_read_new();
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME);
	archive_read_support_format_tar(a);
    archive_read_support_filter_gzip(a);

	if ((r = archive_read_open_filename(a, filename, 10240))) {
		LOG_F(ERROR, "archive_read_open_filename() %s", archive_error_string(a));
        cleanArchive();
        return false;
    }
    
	while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        LOG_F(INFO, "Extracting: %s", archive_entry_pathname(entry));
        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            LOG_F(WARNING, "archive_write_header() %s", archive_error_string(ext));
        } else {
            copy_data(a, ext);
            r = archive_write_finish_entry(ext);
            if (r != ARCHIVE_OK) {
                LOG_F(ERROR, "archive_write_finish_entry() %s", archive_error_string(ext));
                cleanArchive();
                return false;
            }
        }
	}
    if (r != ARCHIVE_OK && r != ARCHIVE_EOF) {
        LOG_F(ERROR, "archive_read_next_header() %s", archive_error_string(a));
        cleanArchive();
        return false;
    }
    
    cleanArchive();

    return true;
}

static int copy_data(struct archive *ar, struct archive *aw) {
	int r;
	const void *buff;
	size_t size;
	int64_t offset;

	while ((r = archive_read_data_block(ar, &buff, &size, &offset)) == ARCHIVE_OK) {
		r = archive_write_data_block(aw, buff, size, offset);
		if (r != ARCHIVE_OK) {
			LOG_F(WARNING, "archive_write_data_block() %s", archive_error_string(aw));
			return r;
		}
	}
    if (r != ARCHIVE_OK) {
        return r;
    }
    return ARCHIVE_OK;
}

}
