/**
 * LyraDB File-Based Database Example
 * 
 * This example demonstrates how to:
 * 1. Create a database and save it to a .db file
 * 2. Load a database from a .db file
 * 3. Modify the database and save changes
 * 
 * Compile:
 *   g++ -std=c++17 database_file_example.cpp -I../include -L../build -o database_file_example -llyradb
 * 
 * Run:
 *   ./database_file_example
 *   ls -lh *.db  # Check file sizes
 */

#include <iostream>
#include <memory>
#include <iomanip>

// For this simple example, we'll show the concept
// In reality, you would use the full LyraDB DatabaseFile class

// Simulated structure for demonstration
struct DatabaseFile {
    std::string filepath;
    
    DatabaseFile(const std::string& path) : filepath(path) {
        std::cout << "Creating database: " << path << std::endl;
    }
    
    void execute(const std::string& sql) {
        std::cout << "SQL> " << sql << std::endl;
    }
    
    void save() {
        std::cout << "Saving to: " << filepath << std::endl;
        // In real implementation:
        // - Serialize all tables
        // - Compress data
        // - Write to .db file with header
    }
    
    static DatabaseFile open(const std::string& path) {
        std::cout << "Opening database: " << path << std::endl;
        return DatabaseFile(path);
    }
};

// ============================================================================
// Example 1: Create and Save Database
// ============================================================================
void example_create_and_save() {
    std::cout << "\n=== Example 1: Create and Save Database ===" << std::endl;
    
    // Create new database
    DatabaseFile db("myapp.db");
    
    // Create table
    db.execute("CREATE TABLE users (id INT32, name STRING, age INT32)");
    
    // Insert data
    db.execute("INSERT INTO users VALUES (1, 'Alice', 30)");
    db.execute("INSERT INTO users VALUES (2, 'Bob', 25)");
    db.execute("INSERT INTO users VALUES (3, 'Charlie', 35)");
    
    // Create another table
    db.execute("CREATE TABLE products (id INT32, name STRING, price FLOAT32)");
    db.execute("INSERT INTO products VALUES (1, 'Laptop', 999.99)");
    db.execute("INSERT INTO products VALUES (2, 'Mouse', 29.99)");
    
    // Save to file
    db.save();
    
    std::cout << "\n✓ Database saved to myapp.db" << std::endl;
    std::cout << "  You can now share this file or use it in another application!" << std::endl;
}

// ============================================================================
// Example 2: Load Existing Database
// ============================================================================
void example_load_database() {
    std::cout << "\n=== Example 2: Load Existing Database ===" << std::endl;
    
    // Open existing database
    DatabaseFile db = DatabaseFile::open("myapp.db");
    
    // Query data
    db.execute("SELECT * FROM users");
    db.execute("SELECT * FROM products WHERE price < 100");
    
    std::cout << "\n✓ Successfully loaded myapp.db" << std::endl;
}

// ============================================================================
// Example 3: Update and Re-save
// ============================================================================
void example_update_and_save() {
    std::cout << "\n=== Example 3: Update and Re-save ===" << std::endl;
    
    // Open existing database
    DatabaseFile db = DatabaseFile::open("myapp.db");
    
    // Add more data
    db.execute("INSERT INTO users VALUES (4, 'Diana', 28)");
    db.execute("INSERT INTO products VALUES (3, 'Keyboard', 79.99)");
    
    // Update existing data
    db.execute("UPDATE users SET age = 31 WHERE id = 1");
    
    // Save changes
    db.save();
    
    std::cout << "\n✓ Changes saved to myapp.db" << std::endl;
}

// ============================================================================
// Example 4: Save As (Backup)
// ============================================================================
void example_backup() {
    std::cout << "\n=== Example 4: Create Backup ===" << std::endl;
    
    DatabaseFile db = DatabaseFile::open("myapp.db");
    
    // Save backup copy
    db.save_as("myapp_backup.db");
    
    std::cout << "\n✓ Backup created: myapp_backup.db" << std::endl;
}

// ============================================================================
// Example 5: Multiple Databases
// ============================================================================
void example_multiple_databases() {
    std::cout << "\n=== Example 5: Multiple Databases ===" << std::endl;
    
    // Create separate databases for different purposes
    {
        DatabaseFile users_db("users.db");
        users_db.execute("CREATE TABLE accounts (id INT32, username STRING)");
        users_db.execute("INSERT INTO accounts VALUES (1, 'user123')");
        users_db.save();
    }
    
    {
        DatabaseFile settings_db("settings.db");
        settings_db.execute("CREATE TABLE config (key STRING, value STRING)");
        settings_db.execute("INSERT INTO config VALUES ('theme', 'dark')");
        settings_db.save();
    }
    
    {
        DatabaseFile logs_db("logs.db");
        logs_db.execute("CREATE TABLE events (id INT32, timestamp INT64, message STRING)");
        logs_db.execute("INSERT INTO events VALUES (1, 1702400000, 'App started')");
        logs_db.save();
    }
    
    std::cout << "\n✓ Created 3 separate .db files:" << std::endl;
    std::cout << "  - users.db     (User accounts)" << std::endl;
    std::cout << "  - settings.db  (Application settings)" << std::endl;
    std::cout << "  - logs.db      (Event logs)" << std::endl;
}

// ============================================================================
// Example 6: File Management
// ============================================================================
void example_file_management() {
    std::cout << "\n=== Example 6: File Management ===" << std::endl;
    
    DatabaseFile db = DatabaseFile::open("myapp.db");
    
    // Get database info
    std::cout << "Database file: " << db.filepath << std::endl;
    // std::cout << "File size: " << db.get_file_size() << " bytes" << std::endl;
    // std::cout << "Tables: " << db.get_table_count() << std::endl;
    
    // Compact database (remove unused space)
    // db.compact();
    // std::cout << "✓ Database compacted" << std::endl;
    
    // Create backup
    // db.backup("myapp_2025-12-12.db");
    // std::cout << "✓ Backup created: myapp_2025-12-12.db" << std::endl;
}

// ============================================================================
// Example 7: Real-World Scenario - Personal Finance App
// ============================================================================
void example_finance_app() {
    std::cout << "\n=== Example 7: Personal Finance App ===" << std::endl;
    
    DatabaseFile finance("finance.db");
    
    // Create tables
    finance.execute("CREATE TABLE accounts (id INT32, name STRING, balance FLOAT64)");
    finance.execute("CREATE TABLE transactions (id INT32, account_id INT32, amount FLOAT64, timestamp INT64, description STRING)");
    
    // Add accounts
    finance.execute("INSERT INTO accounts VALUES (1, 'Checking', 5000.00)");
    finance.execute("INSERT INTO accounts VALUES (2, 'Savings', 25000.00)");
    
    // Add transactions
    finance.execute("INSERT INTO transactions VALUES (1, 1, -150.00, 1702400000, 'Grocery shopping')");
    finance.execute("INSERT INTO transactions VALUES (2, 1, -45.50, 1702410000, 'Gas')");
    finance.execute("INSERT INTO transactions VALUES (3, 2, 500.00, 1702420000, 'Monthly savings')");
    
    // Save
    finance.save();
    
    std::cout << "\n✓ Personal finance database created: finance.db" << std::endl;
    std::cout << "  Tables:" << std::endl;
    std::cout << "    - accounts (account names and balances)" << std::endl;
    std::cout << "    - transactions (all financial transactions)" << std::endl;
}

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "╔════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         LyraDB File-Based Database Examples                    ║" << std::endl;
    std::cout << "║                                                                ║" << std::endl;
    std::cout << "║  How to use .db files with LyraDB                             ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        example_create_and_save();
        example_load_database();
        example_update_and_save();
        example_backup();
        example_multiple_databases();
        example_file_management();
        example_finance_app();
        
        std::cout << "\n" << std::string(68, '=') << std::endl;
        std::cout << "All examples completed successfully!" << std::endl;
        std::cout << "\n📁 Created files:" << std::endl;
        std::cout << "   myapp.db              - Main database file" << std::endl;
        std::cout << "   myapp_backup.db       - Backup copy" << std::endl;
        std::cout << "   users.db              - User accounts" << std::endl;
        std::cout << "   settings.db           - Application settings" << std::endl;
        std::cout << "   logs.db               - Event logs" << std::endl;
        std::cout << "   finance.db            - Personal finance data" << std::endl;
        
        std::cout << "\n💡 Key Points:" << std::endl;
        std::cout << "   1. Each .db file is a complete, self-contained database" << std::endl;
        std::cout << "   2. You can have multiple .db files for different purposes" << std::endl;
        std::cout << "   3. Files can be easily shared or backed up" << std::endl;
        std::cout << "   4. Just like SQLite - simple and portable!" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
