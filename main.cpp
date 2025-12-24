#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <pqxx/pqxx>

using namespace std;

// 1. Получить всех врачей определенной специализации
void getDoctorsBySpecialization(pqxx::connection &conn, const string& specialization)
{
    try
    {
        pqxx::work txn(conn);
        string query = "SELECT last_name || ' ' || first_name || ' ' || middle_name as doctor_name, "
                        "office_number "
                        "FROM doctors "
                        "WHERE specialization = $1" 
                        " ORDER BY last_name";
        
        pqxx::result result = txn.exec_params(query, specialization);
        
        cout << "\n=== Врачи специализации '" << specialization << "' ===\n";
        for (const auto& row : result)
        {
            cout << "ФИО: " << row[0].as<string>()
                            << ", Кабинет: " << row[1].as<string>() << "\n";
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 2. Получить расписание врача на конкретную дату
void getDoctorSchedule(pqxx::connection &conn, string &lastName, string& date)
{
    try
    {
        pqxx::work txn(conn);
        string query = 
            "SELECT d.last_name || ' ' || d.first_name || ' ' || d.middle_name as doctor_name, "
            "d.id, d.specialization, "
            "s.work_date, s.start_time, s.end_time, s.id "
            "FROM doctors d "
            "JOIN schedule s ON s.doctor_id = d.id "
            "WHERE d.last_name = $1 AND s.work_date = $2 AND s.is_available "
            "ORDER BY s.start_time";
        
        pqxx::result result = txn.exec_params(query, lastName, date);
        
        cout << "\n=== Расписание врача на " << date << " ===\n";
        if (result.empty())
        {
            cout << "Расписание не найдено или врач не работает в этот день" << endl;
        }
        for (const auto& row : result)
        {
            cout << "Врач: " << row[0].as<string>()
                        << " (" << row[2].as<string>() << ")\n"
                        << "Время работы: " << row[4].as<string>() 
                        << " - " << row[5].as<string>() << "\n";

            string query = 
                "SELECT a.appointment_time, "
                "p.last_name || ' ' || p.first_name || ' ' || p.middle_name as patient_name "
                "FROM appointments a "
                "JOIN patients p ON a.patient_id = p.id "
                "JOIN doctors d ON a.doctor_id = d.id "
                "JOIN schedule s ON a.schedule_id = s.id "
                "WHERE d.id = $1 AND s.work_date = $2 AND a.schedule_id = $3 "
                "ORDER BY a.appointment_time ";

            pqxx::result result2 = txn.exec_params(query, row[1].as<string>(), date, row[6].as<string>());
            for (const auto& row : result2)
            {
                cout << "Пациент: " << row[1].as<string>()
                     << " - Время приема: " << row[0].as<string>() << "\n";
            }
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 3. Найти все записи пациента
void getPatientAppointments(pqxx::connection &conn, string &lastName)
{
    try {
        pqxx::work txn(conn);
        string query = 
            "SELECT p.last_name || ' ' || p.first_name || ' ' || p.middle_name as patient_name, "
            "d.last_name || ' ' || d.first_name || ' ' || d.middle_name as doctor_name, "
            "d.specialization, d.office_number, "
            "a.appointment_time, s.work_date "
            "FROM patients p "                             
            "JOIN appointments a ON a.patient_id = p.id "
            "JOIN doctors d ON a.doctor_id = d.id "
            "JOIN schedule s ON a.schedule_id = s.id "
            "WHERE p.last_name = $1 "
            "ORDER BY s.work_date DESC, a.appointment_time";
        
        pqxx::result result = txn.exec_params(query, lastName);
        
        cout << "\n=== Записи пациента ===\n";
        for (const auto& row : result)
        {
            cout << "Пациент: " << row[0].as<string>() << "\n"
                        << "Дата: " << row[5].as<string>()
                        << ", Время: " << row[4].as<string>() << "\n"
                        << "Врач: " << row[1].as<string>()
                        << " (" << row[2].as<string>() << ")\n"
                        << "Кабинет: " << row[3].as<string>() 
                        << "\n";
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 4. Получить количество пациентов по полу
void getPatientsCountByGender(pqxx::connection &conn)
{
    try {
        pqxx::work txn(conn);
        string query = 
            "SELECT gender, COUNT(*) as count "
            "FROM patients "
            "GROUP BY gender "
            "ORDER BY count DESC";
        
        pqxx::result result = txn.exec(query);
        
        cout << "\n=== Количество пациентов по полу ===\n";
        for (const auto& row : result)
        {
            string gender = row[0].as<string>() == "М" ? "Мужчины" : "Женщины";
            cout << gender << ": " << row[1].as<int>() << " чел." << "\n";
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 5. Найти врачей с наибольшим количеством записей
void getTopDoctorsByAppointments(pqxx::connection &conn)
{
    try
    {
        pqxx::work txn(conn);
        string query = 
            "SELECT d.id, d.last_name || ' ' || d.first_name || ' ' || d.middle_name as doctor_name, "
            "d.specialization, COUNT(a.id) as appointment_count "
            "FROM appointments a "
            "JOIN doctors d ON a.doctor_id = d.id "
            "JOIN schedule s ON a.schedule_id = s.id "
            "GROUP BY d.id, d.last_name, d.first_name, d.specialization "
            "ORDER BY appointment_count DESC "
            "LIMIT 5";
        
        pqxx::result result = txn.exec(query);
        
        cout << "\n=== Топ-5 врачей по количеству записей ===\n";
        int rank = 1;
        for (const auto& row : result)
        {
            cout << rank++ << ". " << row[1].as<string>()
                        << " (" << row[2].as<string>() << "): "
                        << row[3].as<int>() << " записей" << "\n";
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 6. Найти врачей работающих в определенный день
void getDoctorsByDate(pqxx::connection &conn, string &date)
{
    try
    {
        pqxx::work txn(conn);
        string query = 
            "SELECT s.start_time, s.end_time, " 
            "d.last_name || ' ' || d.first_name || ' ' || d.middle_name as doctor_name, "
            "d.specialization "
            "FROM schedule s "
            "JOIN doctors d on s.doctor_id = d.id "
            "WHERE s.work_date = $1 AND s.is_available ";
        
        pqxx::result result = txn.exec_params(query, date);
        
        cout << "\n=== Врачи работающие " << date << " ===\n";
        for (const auto& row : result) {
            cout << "Врач: " << row[2].as<string>() 
                        << " (" << row[3].as<string>() << ")" << "\n"
                        << "Время работы: " << row[0].as<string>() 
                        << " - " << row[1].as<string>() << "\n";
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 7. Получить медицинские записи пациента с диагнозами
void getPatientMedicalRecords(pqxx::connection &conn, string &lastName) 
{
    try
    {
        pqxx::work txn(conn);
        string query = 
            "SELECT p.last_name || ' ' || p.first_name || ' ' || p.middle_name as patient_name, " 
            "mr.record_date, mr.diagnosis, mr.recommendations, "
            "d.last_name || ' ' || d.first_name || ' ' || d.middle_name as doctor_name, "
            "a.appointment_time "
            "FROM patients p "
            "JOIN medical_records mr ON mr.patient_id = p.id "
            "LEFT JOIN appointments a ON mr.appointment_id = a.id "
            "LEFT JOIN doctors d ON mr.doctor_id = d.id "
            "WHERE p.last_name = $1 "
            "ORDER BY mr.record_date DESC";
        
        pqxx::result result = txn.exec_params(query, lastName);
        
        cout << "\n=== Медицинские записи пациента ===\n";
        for (const auto& row : result)
        {
            cout << "Пациент: " << row[0].as<string>() << "\n"
                        << "Дата: " << row[1].as<string>() 
                        << " (прием в " << (row[5].is_null() ? "N/A" : row[5].as<string>()) << ")\n"
                        << "Врач: " << row[4].as<string>() << "\n"
                        << "Диагноз: " << (row[2].is_null() ? "Нет диагноза" : row[2].as<string>()) << "\n"
                        << "Рекомендации: " << (row[3].is_null() ? "Нет рекомендаций" : row[3].as<string>()) 
                        << "\n" << endl;
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 8. Найти пациентов без медицинских записей
void getPatientsWithoutMedicalRecords(pqxx::connection &conn)
{
    try
    {
        pqxx::work txn(conn);
        string query = 
            "SELECT p.last_name || ' ' || p.first_name || ' ' || p.middle_name as patient_name "
            "FROM patients p "
            "WHERE NOT EXISTS ("
            "  SELECT 1 FROM medical_records mr "
            "  WHERE mr.patient_id = p.id"
            ") "
            "ORDER BY p.last_name ";
        
        pqxx::result result = txn.exec(query);
        
        cout << "\n=== Пациенты без медицинских записей ===\n";
        for (const auto& row : result)
        {
            cout << "ФИО: " << row[0].as<string>() << endl;
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 9. Получить загруженность врачей по дням
void getDoctorsWorkloadByDay(pqxx::connection &conn)
{
    try
    {
        pqxx::work txn(conn);

        string query = 
            "SELECT d.last_name || ' ' || d.first_name || ' ' || d.middle_name as doctor_name, "
            "s.work_date, COUNT(a.id) as appointments_count "
            "FROM doctors d "
            "LEFT JOIN schedule s ON d.id = s.doctor_id "
            "LEFT JOIN appointments a ON s.id = a.schedule_id "
            "WHERE s.work_date >= CURRENT_DATE AND s.work_date <= CURRENT_DATE + INTERVAL '7 days' "
            "GROUP BY d.id, d.last_name, d.first_name, s.work_date "
            "HAVING COUNT(a.id) > 0 "
            "ORDER BY s.work_date, appointments_count DESC";
        
        pqxx::result result = txn.exec(query);
        
        cout << "\n=== Загруженность врачей за последние 7 дней ===\n";
        string current_date;
        for (const auto& row : result)
        {
            string date = row[1].as<string>();
            if (date != current_date) {
                current_date = date;
                cout << "\n" << date << ":\n";
            }
            cout << " - " << row[0].as<string>() << ": " 
                        << row[2].as<int>() << " записей" << endl;
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

// 10. Врачи по кабинету
void getDoctorByOffice(pqxx::connection &conn, int officeNumber)
{
    try
    {
        pqxx::work txn(conn);

        string query = 
            "SELECT d.last_name || ' ' || d.first_name || ' ' || d.middle_name as doctor_name, "
            "d.specialization "
            "FROM doctors d "
            "WHERE office_number = $1";
        
        pqxx::result result = txn.exec_params(query, officeNumber);
        
        cout << "\n=== Врачи в кабинете " << officeNumber << " ===\n";
        if(result.empty())
        {
            cout << "Не найдены\n";
            return;
        }
        for (const auto& row : result)
        {
            cout << "Врач: " << row[0].as<string>() 
                 << " (" << row[1].as<string>() << ")\n";
        }
        txn.commit();
    } catch (const exception& e)
    {
        cerr << "Ошибка запроса: " << e.what() << endl;
    }
}

int main() {

    pqxx::connection *conn;
    try {
        // Подключение к базе данных
        string conn_str = "dbname=polyclinic user=postgres password=12345 host=localhost port=5432";
        conn = new pqxx::connection(conn_str);
        if(!conn->is_open())
        {
            cout << "не удалось подключится\n";
            return 0;
        }
        conn->set_session_var("DateStyle", "ISO, DMY");
    } catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
        return 1;
    }

    while(1)
    {
        cout << "\n1 - Список врачей по специальности\n";
        cout << "2 - Расписание врача на дату\n";
        cout << "3 - Записи пациента\n";
        cout << "4 - Статистика пациентов по полу\n";
        cout << "5 - Топ 5 врачей по записям\n";
        cout << "6 - Список врачей работающих в определенную дату\n";
        cout << "7 - Список диагнозов пациента\n";
        cout << "8 - Пациенты без медицинских записей\n";
        cout << "9 - Загруженность врачей на ближайщие 7 дней\n";
        cout << "10 - Список врачей в кабинете\n";
        cout << "0 - выход\n";
        int input;
        cin >> input;
        switch (input)
        {
        case 1:
        {
            cout << "Введите специальность\n";
            string spec;
            cin >> spec;
            getDoctorsBySpecialization(*conn, spec);
            break;
        }
        case 2:
        {
            cout << "Введите фамилию врача\n";
            string lastName;
            cin >> lastName;
            cout << "Введите дату\n";
            string date;
            cin >> date;
            getDoctorSchedule(*conn, lastName, date);
            break;
        }
        case 3:
        {
            cout << "Введите фамилию пациента\n";
            string lastName;
            cin >> lastName;
            getPatientAppointments(*conn, lastName);
            break;
        }
        case 4:
        {
            getPatientsCountByGender(*conn);
            break;
        }
        case 5:
        {
            getTopDoctorsByAppointments(*conn);
            break;
        }
        case 6:
        {
            cout << "Введите дату\n";
            string date;
            cin >> date;
            getDoctorsByDate(*conn, date);
            break;
        }
        case 7:
        {
            cout << "Введите фамилию пациента\n";
            string lastName;
            cin >> lastName;
            getPatientMedicalRecords(*conn, lastName);
            break;
        }
        case 8:
        {
            getPatientsWithoutMedicalRecords(*conn);
            break;
        }
        case 9:
        {
            getDoctorsWorkloadByDay(*conn);
            break;
        }
        case 10:
        {
            cout << "Введите номер кабинета\n";
            int officeNumber;
            cin >> officeNumber;
            getDoctorByOffice(*conn, officeNumber);
            break;
        }
        case 0:
        {
            return 0;
        }
        default:
            break;
        }
    }

    return 0;
}