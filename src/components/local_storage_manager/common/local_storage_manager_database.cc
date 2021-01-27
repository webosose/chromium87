// Copyright (c) 2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "components/local_storage_manager/common/local_storage_manager_database.h"

#include "base/logging.h"

namespace content {

const base::FilePath::CharType kLocalStorageManagerDataFileName[] =
    FILE_PATH_LITERAL("AppsOrigins");

LocalStorageManagerDatabase::LocalStorageManagerDatabase(
    const base::FilePath& data_file_name)
    : data_file_name_(data_file_name.Append(kLocalStorageManagerDataFileName)) {
}

bool LocalStorageManagerDatabase::AddAccess(const AccessData& access) {
  sql::Statement statement(
      db_.GetCachedStatement(SQL_FROM_HERE,
                             "INSERT INTO access "
                             "(id_app, id_origin) SELECT applications.id, "
                             "origins.id FROM applications, "
                             "origins WHERE applications.app_id=? AND "
                             "origins.url=?"));
  statement.BindString(0, access.app_id_);
  statement.BindString(1, access.origin_.spec());
  if (!statement.Run()) {
    LOG(ERROR) << "Failed to execute access insert statement:  "
               << "app_id = " << access.app_id_
               << "; origin=" << access.origin_;
    return false;
  }
  return true;
}

bool LocalStorageManagerDatabase::AddApplication(
    const ApplicationData& application) {
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE,
      "INSERT INTO applications (app_id, installed) VALUES(?, ?)"));
  statement.BindString(0, application.app_id_);
  statement.BindInt(1, application.installed_ ? 1 : 0);
  if (!statement.Run()) {
    LOG(ERROR) << "Failed to execute application insert statement:  "
               << "app_id = " << application.app_id_
               << "; installed=" << application.installed_;
    return false;
  }
  return true;
}

bool LocalStorageManagerDatabase::AddOrigin(const OriginData& origin) {
  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE, "INSERT INTO origins (url) VALUES(?)"));
  statement.BindString(0, origin.url_.spec());
  if (!statement.Run()) {
    LOG(ERROR) << "Failed to execute origin insert statement:  "
               << "url = " << origin.url_;
    return false;
  }
  return true;
}

bool LocalStorageManagerDatabase::GetAccesses(AccessDataList* accesses) {
  if (accesses == nullptr)
    return false;

  sql::Statement statement(
      db_.GetCachedStatement(SQL_FROM_HERE,
                             "SELECT applications.app_id, origins.url FROM "
                             "applications, origins, access "
                             "WHERE access.id_app = applications.id AND "
                             "access.id_origin = origins.id"));
  if (!statement.is_valid())
    return false;
  while (statement.Step()) {
    AccessData access;
    access.app_id_ = statement.ColumnString(0);
    access.origin_ = GURL(statement.ColumnString(1));
    accesses->push_back(access);
  }
  return true;
}

bool LocalStorageManagerDatabase::GetApplications(
    ApplicationDataList* applications) {
  if (applications == nullptr)
    return false;

  sql::Statement statement(db_.GetCachedStatement(
      SQL_FROM_HERE, "SELECT app_id, installed FROM applications"));
  if (!statement.is_valid())
    return false;
  while (statement.Step()) {
    ApplicationData application;
    application.app_id_ = statement.ColumnString(0);
    application.installed_ = statement.ColumnInt(1) != 0;
    applications->push_back(application);
  }
  return true;
}

bool LocalStorageManagerDatabase::DeleteApplication(const std::string& app_id) {
  sql::Transaction committer(&db_);
  if (!committer.Begin())
    return false;

  sql::Statement statement_access(db_.GetCachedStatement(
      SQL_FROM_HERE,
      "DELETE FROM access WHERE id_app IN (SELECT id FROM "
      "applications WHERE app_id=(?))"));
  statement_access.BindString(0, app_id);
  if (!statement_access.Run()) {
    LOG(ERROR) << "Failed to execute access delete statement; app_id="
               << app_id;
    return false;
  }

  sql::Statement statement_app(db_.GetCachedStatement(
      SQL_FROM_HERE, "DELETE FROM applications WHERE app_id=(?)"));
  statement_app.BindString(0, app_id);
  if (!statement_app.Run()) {
    LOG(ERROR) << "Failed to execute application delete statement:  "
               << "app_id = " << app_id;
    return false;
  }

  return committer.Commit();
}

bool LocalStorageManagerDatabase::DeleteOrigin(const GURL& url) {
  sql::Transaction committer(&db_);
  if (!committer.Begin())
    return false;

  sql::Statement statement_access(db_.GetCachedStatement(
      SQL_FROM_HERE,
      "DELETE FROM access WHERE id_origin IN (SELECT id FROM "
      "origins WHERE origins.url=(?))"));
  statement_access.BindString(0, url.spec());
  if (!statement_access.Run()) {
    LOG(ERROR) << "Failed to execute access delete statement; origin=" << url;
    return false;
  }

  sql::Statement statement_origin(db_.GetCachedStatement(
      SQL_FROM_HERE, "DELETE FROM origins WHERE url=(?)"));
  statement_origin.BindString(0, url.spec());
  if (!statement_origin.Run()) {
    LOG(ERROR) << "Failed to execute origin delete statement:  "
               << "Origin = " << url;
    return false;
  }

  return committer.Commit();
}

sql::InitStatus LocalStorageManagerDatabase::Init() {
  if (!EnsurePath(data_file_name_)) {
    LOG(ERROR) << "Invalid DB path=" << data_file_name_.AsUTF8Unsafe();
    return sql::INIT_FAILURE;
  }
  db_.set_histogram_tag("LGE_LocalStorageManager");

  // Set the database page size to something a little larger to give us
  // better performance (we're typically seek rather than bandwidth limited).
  // This only has an effect before any tables have been created, otherwise
  // this is a NOP. Must be a power of 2 and a max of 8192.
  db_.set_page_size(4096);

  // Set the cache size. The page size, plus a little extra, times this
  // value, tells us how much memory the cache will use maximum.
  // 100 * 4kB = 400kB
  // TODO(brettw) scale this value to the amount of available memory.
  db_.set_cache_size(100);

  // Note that we don't set exclusive locking here. That's done by
  // BeginExclusiveMode below which is called later (we have to be in shared
  // mode to start out for the in-memory backend to read the data).

  if (!db_.Open(data_file_name_))
    return sql::INIT_FAILURE;

  // Wrap the rest of init in a tranaction. This will prevent the database from
  // getting corrupted if we crash in the middle of initialization or migration.
  sql::Transaction committer(&db_);
  if (!committer.Begin())
    return sql::INIT_FAILURE;

  // Prime the cache.
  db_.Preload();
  if (!CreateAppsTable() || !CreateOriginsTable() ||
      !CreateLocalStorageAccessTable()) {
    return sql::INIT_FAILURE;
  }
  return committer.Commit() ? sql::INIT_OK : sql::INIT_FAILURE;
}

bool LocalStorageManagerDatabase::CreateAppsTable() {
  if (!db_.DoesTableExist("applications")) {
    if (!db_.Execute("CREATE TABLE applications("
                     "id INTEGER PRIMARY KEY,"
                     "app_id LONGVARCHAR NOT NULL,"
                     "installed INTEGER DEFAULT 0 NOT NULL)"))
      return false;
  }
  return true;
}

bool LocalStorageManagerDatabase::CreateLocalStorageAccessTable() {
  if (!db_.DoesTableExist("access")) {
    if (!db_.Execute("CREATE TABLE access("
                     "id_app INTEGER NOT NULL,"
                     "id_origin INTEGER NOT NULL)"))
      return false;
    if (!db_.Execute("CREATE INDEX IF NOT EXISTS access_apps_index ON "
                     "access (id_app)")) {
      return false;
    }
    if (!db_.Execute("CREATE INDEX IF NOT EXISTS access_origins_index ON "
                     "access (id_origin)")) {
      return false;
    }
  }
  return true;
}

bool LocalStorageManagerDatabase::CreateOriginsTable() {
  if (!db_.DoesTableExist("origins")) {
    if (!db_.Execute("CREATE TABLE origins("
                     "id INTEGER PRIMARY KEY,"
                     "url LONGVARCHAR NOT NULL)"))
      return false;
  }
  return true;
}

bool LocalStorageManagerDatabase::EnsurePath(const base::FilePath& path) {
  if (!base::PathExists(path) && !base::CreateDirectory(path.DirName())) {
    LOG(ERROR) << "Failed to create directory: " << path.LossyDisplayName();
    return false;
  }
  return true;
}
}  // namespace content
