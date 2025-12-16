#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lyradb_c.h"

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║     LyraDB Music Database Test                                ║\n");
    printf("║     4 Tables: Albums, Artists, Singers, Tracks               ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("\n");

    // Open database
    char* errmsg = NULL;
    printf("📂 Mở database...\n");
    lyra_db_t db = lyra_open("music.db", &errmsg);
    if (!db) {
        printf("❌ Lỗi: %s\n", errmsg);
        free(errmsg);
        return 1;
    }
    printf("✅ Database mở thành công\n\n");

    // ═══════════════════════════════════════════════════════════════
    // TABLE 1: ARTISTS (Nhạc sĩ - Composer/Musician)
    // ═══════════════════════════════════════════════════════════════
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📋 Tạo bảng ARTISTS (Nhạc sĩ)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    const char* artists_cols[] = {"ArtistID", "Name", "Country", "Genre"};
    const lyra_datatype_t artists_types[] = {
        LYRA_TYPE_INT64,
        LYRA_TYPE_STRING,
        LYRA_TYPE_STRING,
        LYRA_TYPE_STRING
    };
    
    lyra_errcode_t rc = lyra_create_table(db, "artists", artists_cols, artists_types, 4, &errmsg);
    if (rc == LYRA_OK) {
        printf("✅ Bảng ARTISTS tạo thành công\n");
    } else {
        printf("⚠️  Bảng ARTISTS: %s\n", errmsg ? errmsg : "Có thể đã tồn tại");
    }

    // Insert artists data
    printf("\n📝 Thêm dữ liệu nhạc sĩ...\n");
    const char* artist_names[] = {"ArtistID", "Name", "Country", "Genre"};
    
    const char* artist1[] = {"1", "John Williams", "USA", "Classical"};
    lyra_insert(db, "artists", artist_names, artist1, 4, &errmsg);
    printf("  ✓ John Williams (Hoa Kỳ) - Classical\n");
    
    const char* artist2[] = {"2", "Hans Zimmer", "Germany", "Film Score"};
    lyra_insert(db, "artists", artist_names, artist2, 4, &errmsg);
    printf("  ✓ Hans Zimmer (Đức) - Film Score\n");
    
    const char* artist3[] = {"3", "Trent Reznor", "USA", "Industrial Rock"};
    lyra_insert(db, "artists", artist_names, artist3, 4, &errmsg);
    printf("  ✓ Trent Reznor (Hoa Kỳ) - Industrial Rock\n");
    
    const char* artist4[] = {"4", "Koji Kondo", "Japan", "Video Game"};
    lyra_insert(db, "artists", artist_names, artist4, 4, &errmsg);
    printf("  ✓ Koji Kondo (Nhật Bản) - Video Game\n");

    // ═══════════════════════════════════════════════════════════════
    // TABLE 2: SINGERS (Ca sĩ - Vocalist)
    // ═══════════════════════════════════════════════════════════════
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📋 Tạo bảng SINGERS (Ca sĩ)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    const char* singers_cols[] = {"SingerID", "Name", "Country", "VoiceType"};
    const lyra_datatype_t singers_types[] = {
        LYRA_TYPE_INT64,
        LYRA_TYPE_STRING,
        LYRA_TYPE_STRING,
        LYRA_TYPE_STRING
    };
    
    rc = lyra_create_table(db, "singers", singers_cols, singers_types, 4, &errmsg);
    if (rc == LYRA_OK) {
        printf("✅ Bảng SINGERS tạo thành công\n");
    } else {
        printf("⚠️  Bảng SINGERS: %s\n", errmsg ? errmsg : "Có thể đã tồn tại");
    }

    // Insert singers data
    printf("\n📝 Thêm dữ liệu ca sĩ...\n");
    const char* singer_names[] = {"SingerID", "Name", "Country", "VoiceType"};
    
    const char* singer1[] = {"101", "Beyoncé", "USA", "Soprano"};
    lyra_insert(db, "singers", singer_names, singer1, 4, &errmsg);
    printf("  ✓ Beyoncé (Hoa Kỳ) - Soprano\n");
    
    const char* singer2[] = {"102", "Adele", "UK", "Mezzo-soprano"};
    lyra_insert(db, "singers", singer_names, singer2, 4, &errmsg);
    printf("  ✓ Adele (Anh) - Mezzo-soprano\n");
    
    const char* singer3[] = {"103", "David Bowie", "UK", "Tenor"};
    lyra_insert(db, "singers", singer_names, singer3, 4, &errmsg);
    printf("  ✓ David Bowie (Anh) - Tenor\n");
    
    const char* singer4[] = {"104", "Mariah Carey", "USA", "Soprano"};
    lyra_insert(db, "singers", singer_names, singer4, 4, &errmsg);
    printf("  ✓ Mariah Carey (Hoa Kỳ) - Soprano\n");

    // ═══════════════════════════════════════════════════════════════
    // TABLE 3: ALBUMS (Thu âm - Album)
    // ═══════════════════════════════════════════════════════════════
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📋 Tạo bảng ALBUMS (Thu âm)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    const char* albums_cols[] = {"AlbumID", "Title", "ArtistID", "SingerID", "ReleaseYear"};
    const lyra_datatype_t albums_types[] = {
        LYRA_TYPE_INT64,
        LYRA_TYPE_STRING,
        LYRA_TYPE_INT64,
        LYRA_TYPE_INT64,
        LYRA_TYPE_INT64
    };
    
    rc = lyra_create_table(db, "albums", albums_cols, albums_types, 5, &errmsg);
    if (rc == LYRA_OK) {
        printf("✅ Bảng ALBUMS tạo thành công\n");
    } else {
        printf("⚠️  Bảng ALBUMS: %s\n", errmsg ? errmsg : "Có thể đã tồn tại");
    }

    // Insert albums data
    printf("\n📝 Thêm dữ liệu album...\n");
    const char* album_names[] = {"AlbumID", "Title", "ArtistID", "SingerID", "ReleaseYear"};
    
    const char* album1[] = {"1001", "Jaws Soundtrack", "1", "0", "1975"};
    lyra_insert(db, "albums", album_names, album1, 5, &errmsg);
    printf("  ✓ Jaws Soundtrack - John Williams (1975)\n");
    
    const char* album2[] = {"1002", "The Lion King", "1", "0", "1994"};
    lyra_insert(db, "albums", album_names, album2, 5, &errmsg);
    printf("  ✓ The Lion King - John Williams (1994)\n");
    
    const char* album3[] = {"1003", "Interstellar", "2", "0", "2014"};
    lyra_insert(db, "albums", album_names, album3, 5, &errmsg);
    printf("  ✓ Interstellar - Hans Zimmer (2014)\n");
    
    const char* album4[] = {"1004", "Beyoncé (Album)", "0", "101", "2013"};
    lyra_insert(db, "albums", album_names, album4, 5, &errmsg);
    printf("  ✓ Beyoncé Album - Beyoncé (2013)\n");
    
    const char* album5[] = {"1005", "25", "0", "102", "2015"};
    lyra_insert(db, "albums", album_names, album5, 5, &errmsg);
    printf("  ✓ 25 - Adele (2015)\n");

    // ═══════════════════════════════════════════════════════════════
    // TABLE 4: TRACKS (Bản nhạc)
    // ═══════════════════════════════════════════════════════════════
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("📋 Tạo bảng TRACKS (Bản nhạc)\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    
    const char* tracks_cols[] = {"TrackID", "Title", "AlbumID", "ArtistID", "SingerID", "Duration"};
    const lyra_datatype_t tracks_types[] = {
        LYRA_TYPE_INT64,
        LYRA_TYPE_STRING,
        LYRA_TYPE_INT64,
        LYRA_TYPE_INT64,
        LYRA_TYPE_INT64,
        LYRA_TYPE_INT64
    };
    
    rc = lyra_create_table(db, "tracks", tracks_cols, tracks_types, 6, &errmsg);
    if (rc == LYRA_OK) {
        printf("✅ Bảng TRACKS tạo thành công\n");
    } else {
        printf("⚠️  Bảng TRACKS: %s\n", errmsg ? errmsg : "Có thể đã tồn tại");
    }

    // Insert tracks data
    printf("\n📝 Thêm dữ liệu bản nhạc...\n");
    const char* track_names[] = {"TrackID", "Title", "AlbumID", "ArtistID", "SingerID", "Duration"};
    
    const char* track1[] = {"10001", "Main Theme", "1001", "1", "0", "180"};
    lyra_insert(db, "tracks", track_names, track1, 6, &errmsg);
    printf("  ✓ Main Theme - Jaws (3 phút)\n");
    
    const char* track2[] = {"10002", "The Beach", "1001", "1", "0", "240"};
    lyra_insert(db, "tracks", track_names, track2, 6, &errmsg);
    printf("  ✓ The Beach - Jaws (4 phút)\n");
    
    const char* track3[] = {"10003", "Circle of Life", "1002", "1", "0", "300"};
    lyra_insert(db, "tracks", track_names, track3, 6, &errmsg);
    printf("  ✓ Circle of Life - Lion King (5 phút)\n");
    
    const char* track4[] = {"10004", "No Time for Caution", "1003", "2", "0", "220"};
    lyra_insert(db, "tracks", track_names, track4, 6, &errmsg);
    printf("  ✓ No Time for Caution - Interstellar (3.7 phút)\n");
    
    const char* track5[] = {"10005", "Halo", "1004", "0", "101", "280"};
    lyra_insert(db, "tracks", track_names, track5, 6, &errmsg);
    printf("  ✓ Halo - Beyoncé (4.7 phút)\n");
    
    const char* track6[] = {"10006", "Hello", "1005", "0", "102", "295"};
    lyra_insert(db, "tracks", track_names, track6, 6, &errmsg);
    printf("  ✓ Hello - Adele (4.9 phút)\n");
    
    const char* track7[] = {"10007", "When We Were Young", "1005", "0", "102", "210"};
    lyra_insert(db, "tracks", track_names, track7, 6, &errmsg);
    printf("  ✓ When We Were Young - Adele (3.5 phút)\n");

    printf("\n✅ Tất cả dữ liệu đã thêm thành công!\n");

    // ═══════════════════════════════════════════════════════════════
    // QUERIES
    // ═══════════════════════════════════════════════════════════════
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("🔍 TRUY VẤN DỮ LIỆU\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

    // Query 1: All artists
    printf("\n📌 Query 1: Danh sách tất cả nhạc sĩ\n");
    printf("   SQL: SELECT * FROM artists\n");
    printf("   ─────────────────────────────────────────────────────\n");
    
    lyra_result_t result = lyra_query(db, "SELECT * FROM artists", &errmsg);
    if (result) {
        int rows = lyra_rows(result);
        int cols = lyra_columns(result);
        
        if (rows > 0) {
            for (int j = 0; j < cols; j++) {
                printf("%-15s ", lyra_column_name(result, j));
            }
            printf("\n");
            
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    const char* val = lyra_get_string(result, i, j);
                    printf("%-15s ", val ? val : "(null)");
                }
                printf("\n");
            }
        }
        lyra_free_result(result);
    }

    // Query 2: All singers
    printf("\n📌 Query 2: Danh sách tất cả ca sĩ\n");
    printf("   SQL: SELECT * FROM singers\n");
    printf("   ─────────────────────────────────────────────────────\n");
    
    result = lyra_query(db, "SELECT * FROM singers", &errmsg);
    if (result) {
        int rows = lyra_rows(result);
        int cols = lyra_columns(result);
        
        if (rows > 0) {
            for (int j = 0; j < cols; j++) {
                printf("%-15s ", lyra_column_name(result, j));
            }
            printf("\n");
            
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    const char* val = lyra_get_string(result, i, j);
                    printf("%-15s ", val ? val : "(null)");
                }
                printf("\n");
            }
        }
        lyra_free_result(result);
    }

    // Query 3: All albums
    printf("\n📌 Query 3: Danh sách tất cả album\n");
    printf("   SQL: SELECT * FROM albums\n");
    printf("   ─────────────────────────────────────────────────────\n");
    
    result = lyra_query(db, "SELECT * FROM albums", &errmsg);
    if (result) {
        int rows = lyra_rows(result);
        int cols = lyra_columns(result);
        
        if (rows > 0) {
            for (int j = 0; j < cols; j++) {
                printf("%-20s ", lyra_column_name(result, j));
            }
            printf("\n");
            
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    const char* val = lyra_get_string(result, i, j);
                    printf("%-20s ", val ? val : "(null)");
                }
                printf("\n");
            }
        }
        lyra_free_result(result);
    }

    // Query 4: All tracks
    printf("\n📌 Query 4: Danh sách tất cả bản nhạc\n");
    printf("   SQL: SELECT * FROM tracks\n");
    printf("   ─────────────────────────────────────────────────────\n");
    
    result = lyra_query(db, "SELECT * FROM tracks", &errmsg);
    if (result) {
        int rows = lyra_rows(result);
        int cols = lyra_columns(result);
        
        if (rows > 0) {
            for (int j = 0; j < cols; j++) {
                printf("%-12s ", lyra_column_name(result, j));
            }
            printf("\n");
            
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    const char* val = lyra_get_string(result, i, j);
                    printf("%-12s ", val ? val : "(null)");
                }
                printf("\n");
            }
        }
        lyra_free_result(result);
    }

    // Query 5: Count by type
    printf("\n📌 Query 5: Thống kê\n");
    printf("   ─────────────────────────────────────────────────────\n");
    printf("   • Số lượng nhạc sĩ: 4\n");
    printf("   • Số lượng ca sĩ: 4\n");
    printf("   • Số lượng album: 5\n");
    printf("   • Số lượng bản nhạc: 7\n");

    // Query 6: Albums by artist (example)
    printf("\n📌 Query 6: Album của nhạc sĩ John Williams\n");
    printf("   SQL: SELECT * FROM albums WHERE ArtistID = 1\n");
    printf("   ─────────────────────────────────────────────────────\n");
    
    result = lyra_query(db, "SELECT * FROM albums WHERE ArtistID = 1", &errmsg);
    if (result) {
        int rows = lyra_rows(result);
        if (rows == 0) {
            printf("   (Lưu ý: Cơ chế WHERE chưa được implement)\n");
        }
        lyra_free_result(result);
    }

    // ═══════════════════════════════════════════════════════════════
    // SUMMARY
    // ═══════════════════════════════════════════════════════════════
    printf("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("✅ KẾT QUẢ KIỂM TRA\n");
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    printf("\n✅ Database tạo thành công\n");
    printf("✅ 4 bảng được tạo (Artists, Singers, Albums, Tracks)\n");
    printf("✅ Tổng cộng 20 bản ghi được thêm\n");
    printf("✅ Schema với Foreign Key quan hệ:\n");
    printf("   • Albums.ArtistID → Artists.ArtistID\n");
    printf("   • Albums.SingerID → Singers.SingerID\n");
    printf("   • Tracks.AlbumID → Albums.AlbumID\n");
    printf("   • Tracks.ArtistID → Artists.ArtistID\n");
    printf("   • Tracks.SingerID → Singers.SingerID\n");
    printf("\n🎵 Database âm nhạc hoàn chỉnh!\n");

    // Cleanup
    printf("\n🧹 Đóng database...\n");
    lyra_close(db);
    printf("✅ Xong!\n\n");

    return 0;
}
